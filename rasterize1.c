#include "def.h"

#define RAST1_FIXSHIFT 10
#define RAST1_FIX (1 << RAST1_FIXSHIFT) // (1 << 10) == 1024
#define RAST1_FIXMASK (RAST1_FIX - 1)

/*
* NOTE(chan)
* start_point is the y coordinate of the scanline.
* The reason why we get dx/dy instead of dy/dx is that
* we fill the scanline (width, x) with the edge so that
* we can increment x with dx.
* 
* I guess the reason Sean use RAST1_FIX is to avoid using a very small floating-point
* that could cause an unaccurate operation.
* To calculate the start x of the active edge, you multiply dx with 'start_point - e->y0'.
*/
static ActiveEdge* rast1_new_active(Heap* h, Edge* e, float start_point)
{
    ActiveEdge* z = (ActiveEdge*)heap_alloc(h, sizeof(*z));
    float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    assert(z != NULL);
    if (!z) return z;
    
    // NOTE(sean) : round dx down to avoid overshooting
    if(dxdy < 0)
        z->dx = -IFLOOR(RAST1_FIX * -dxdy);
    else
        z->dx = IFLOOR(RAST1_FIX * dxdy);
    
    z->x = IFLOOR(RAST1_FIX * e->x0 + z->dx * (start_point - e->y0));
    
    z->ey = e->y1;
    z->next = 0;
    z->direction = e->invert ? 1.f : -1.f;
    return z;
}

/*
* NOTE(chan): !!core function!!
* 
* Explanation from https://nothings.org/gamedev/rasterize/
* Determine if a point is within a concave-with-holes polygon.
* To do that, classify each polygon edge with a direction (1 or -1).
* Cast a ray from the point to infinity (in the horizon direction), 
* and add the direction of edges the ray crosses.
* If the final sum is non-zero, then the point is inside the polygon.
* 
* If I understand correctly, you cast two rays from a point toward 
* the left side and the right side. If the direction number of edges 
* from left/right sides sums to 0, 
* it means the pixel is inside in the polygon, therefore it can be filled.
* To simplify this algorithm more, we have the list of intersection points
 * (sorted in x coordinates) with the scanline 
* instead of casting rays every time. So, 
* If the direction value of the two pairs sums to 0, 
* it means the pixels between two intersection points should be filled.
*
* TODO(chan) : study more about combined coverage and anti aliasing.
*/
static void rast1_fill_active(unsigned char* scanline, int len, ActiveEdge* e, int max_weight)
{
    // non-zero winding fill
    int x0 = 0, w = 0;
    
    while(e)
    {
        if (w == 0)
        {
            // if we're currently at zero, we need to record the edge start point
            x0 = e->x; w += e->direction;
        }
        else
        {
            int x1 = e->x; w += e->direction;
            
            // if we went to zero, we need to draw
            if (w == 0)
            {
                int i = x0 >> RAST1_FIXSHIFT;
                int j = x1 >> RAST1_FIXSHIFT;
                
                if (i < len && j >= 0)
                {
                    if (i == j)
                    {
                        // x0, x1 are the same pixel, so compute comibned coverage
                        scanline[i] = scanline[i] + (uint8_t)(((x1 - x0) * max_weight) >> RAST1_FIXSHIFT);
                    }
                    else
                    {
                        if (i >= 0) // add antialiasing for x0
                            scanline[i] = scanline[i] + (uint8_t)(((RAST1_FIX - (x0 & RAST1_FIXMASK)) * max_weight) >> RAST1_FIXSHIFT);
                        else
                            i = -1; // clip
                        
                        if (j < len) // add antialiasing for x1
                            scanline[j] = scanline[j] + (uint8_t)(((x1 & RAST1_FIXMASK) * max_weight) >> RAST1_FIXSHIFT);
                        else
                            j = len; // clip
                        
                        for(++i; i < j; ++i) // fill pixels between x0 and x1
                            scanline[i] = scanline[i] + (uint8_t)max_weight;
                    }
                }
            }
        }
        
        e = e->next;
    }
}

void canvas_rasterize1_sorted_edges(Canvas* canvas, Edge* e, int edge_count, int vsubsample)
{
    Heap hh = {0, 0, 0};
    int stride = canvas->w * canvas->comp;
    int j = 0;
    int y = 0 * vsubsample; // NOTE(chan) : the original code use offset for glyph, but I'm not using glyph here. So I use it as zero.
    int max_weight = (255 / vsubsample); 
    int s; // vertical subsample index
    ActiveEdge* active = NULL;
    
    // this edge array has one more element for sentinel
    // refer to edges_alloc_for_raster_from_polygon(~).
    Edge* sentinel = e + edge_count;
    // e[edge_count].y0 = canvas->h * vsubsample;
    uint8_t* scanline =  (uint8_t*)malloc(canvas->w);
    
    while(j < canvas->h)
    {
        memset(scanline, 0, canvas->w);
        // NOTE(chan) : as long as you use higher vsubsample, 
        // there would be more active edgese in the current scanline,
        // and it will be likely to fill more pixels.
        for(s = 0; s < vsubsample; ++s) 
        {
            float scan_y = y + 0.5f; // we check the center height of the pixel
            ActiveEdge** step = &active;
            
            // NOTE(sean) : update all active edges;
            // remove all active edges that terminate before the center of this scanline
            // NOTE(chan) : Algorithm 3-2 for `if (z->ey <= scan_y)`
            //              Algorithm 3-5 for 'else'
            while(*step)
            {
                ActiveEdge* z = *step;
                // NOTE(chan) : remember z->ey is the y1 of Edge, where
                // y1 is always bigger that y0.
                // so if (z->ey <= scan_y) is true, then we don't need to care this edge any more.
                if (z->ey <= scan_y)
                {
                    *step = z->next; // delete from list
                    assert(z->direction != 0.f);
                    z->direction = 0;
                    heap_free(&hh, z);
                }
                else
                {
                    z->x += z->dx; // advance to position for current scanline
                    step = &((*step)->next);
                }
            }
            
            // NOTE(sean) : resort the list if needed
            // NOTE(chan) : Algorithm 3-3
            for(;;)
            {
                int changed = 0;
                step = &active;
                while (*step && (*step)->next)
                {
                    if((*step)->x > (*step)->next->x)
                    {
                        ActiveEdge* t = *step;
                        ActiveEdge* q = t->next;
                        
                        t->next = q->next;
                        q->next = t;
                        *step = q;
                        changed = 1;
                    }
                    step = &(*step)->next;
                }
                if (!changed) break;
            }
            
            // Algorithm 3-1
            // NOTE(sean) : insert all edges that start before the center of this scanline
            // omit ones that also end on this scanline
            // NOTE(chan) : it was `while (e->y0 <= scan_y)`,
            // but if the vsubsample is high enough, then 
            // the while condition is met and it will insert unallocated edges.
            // So I check the sentinel directly here.
            // Accordingto the algorithm 3-1, the ActiveEdge.x is the intersection with the scanline.
            while(e != sentinel && e->y0 <= scan_y) 
                // while (e->y0 <= scan_y)
            {
                if(e->y1 > scan_y)
                {
                    ActiveEdge* z = rast1_new_active(&hh, e, scan_y);
                    if(z != NULL)
                    {
                        // find insertion point
                        if (active == NULL)
                            active = z;
                        else if(z->x < active->x)
                        {
                            // insert at front
                            z->next = active;
                            active = z;
                        }
                        else
                        {
                            // find thing to insert AFTER
                            ActiveEdge* p = active;
                            while(p->next && p->next->x < z->x)
                                p = p->next;
                            // at this point, p->next->x is NOT < z->x
                            z->next = p->next;
                            p->next = z;
                        }
                    }
                }
                ++e;
            }
            
            // NOTE(chan) : Algorithm 3-4
            if (active)
                rast1_fill_active(scanline, canvas->w, active, max_weight);
            
            ++y;
        }
        
        memcpy(canvas->p + j * stride, scanline, canvas->w);
        ++j;
    }
    
    heap_cleanup(&hh);
    
    free(scanline);
}