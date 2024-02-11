#include "def.h"

Edge* edges_alloc_for_raster_from_polygon(Polygon* p, float scale_x, float scale_y, float shift_x, float shift_y, int invert, int vsubsample, int* out_edge_count)
{
    float y_scale_inv = invert ? -scale_y : scale_y;
    Edge* edges = (Edge*)malloc(sizeof(Edge) * (p->count + 1)); // add an extra one as a sentinel
    
    /*
* NOTE(chan)
* I assume the start point of the edge is x0, y0, and
* the end point of the edge is x1, y1.
* 
* This function constructs an edge with the start 'a' and the end 'b'.
* The numbering of edges from a polygon works like this:
* edges[0] = (V_0, V_{n-1}) // V_{polygon vertex index} / (start, end)
* edges[1] = (V_1, V_0)
* edges[2] = (V_2, V_1)
* ...
* edges[n - 1] = (v_{n-1}, v_{n-2})
* However, if the y coordinate of an start point is bigger than the y coordinate of its end point, then
* The start point and the end point are swapped. 
* In addition, you set the invert variable of the edge as 1 to use it later for the scan algorithm.
* Therefore, the y coordinate of an end point is always bigger than the y coordinate of its start point.
* The condition can be inverted with the invert parameter not being 0.
*/
    int edge_n = 0;
    int j = p->count - 1;
    for(int k = 0; k < p->count;j=k++)
    {
        int a = k, b = j; // a : start point, b : end point
        
        // NOTE(sean) : skip the edge if horizontal
        if (p->vertices[a].y == p->vertices[b].y)
            continue;
        
        edges[edge_n].invert = 0;
        if(invert ? p->vertices[b].y > p->vertices[a].y : p->vertices[b].y < p->vertices[a].y)
        {
            edges[edge_n].invert = 1;
            a = j, b = k;
        }
        
        edges[edge_n].x0 = p->vertices[a].x * scale_x + shift_x;
        edges[edge_n].y0 = (p->vertices[a].y * y_scale_inv + shift_y) * vsubsample;
        edges[edge_n].x1 = p->vertices[b].x * scale_x + shift_x;
        edges[edge_n].y1 = (p->vertices[b].y * y_scale_inv + shift_y) * vsubsample;
        ++edge_n;
    }
    
    *out_edge_count = edge_n;
    
    return edges;
}

void edges_free(Edge* edges)
{
    free(edges);
}

/*
* sort edges in the insertion algorithm.
* It compare the y coordinate of the start point of the edge.
* The lower y value of an edge will be located from the first in the array.
*/
static void edges_sort_insertion(Edge* edges, int count)
{
    int i, j;
    for(i = 1; i < count; ++i)
    {
        Edge t = edges[i];
        Edge* a = &t;
        j = i;
        while(j > 0)
        {
            Edge* b = &edges[j - 1];
            int c = (a->y0 < b->y0);
            if(!c) break;
            edges[j] = edges[j - 1];
            --j;
        }
        
        if(i != j)
            edges[j] = t;
    }
}

static void edges_sort_quick(Edge* edges, int count)
{
    while(count > 12) // threshold for transitioning to insertion sort
    {
        Edge t;
        int c01, c12, c, m, i, j;
        
        /*
* NOTE(chan) : According to wikipedia, this strategy looks similar to the Median-of-three code.
*/
        
        m = count >> 1; // median
        c01 = (edges[0].y0 < edges[m].y0);
        c12 = (edges[m].y0 < edges[count - 1].y0);
        
        // if 0 >= mid >= end, or 0 < mid < end, then use mid
        if(c01 != c12)
        {
            /* otherwise, we'll need to swap someting else to middle*/
            int z;
            c = (edges[0].y0 < edges[count - 1].y0);
            /*
            * case 0
            * - c01 = 1 -> 0 < m
* - c12 = 0 -> m > n
* - c = 0   -> n < 0 < m => z = 0 => n < m < 0 after swap
* - c = 1   -> 0 < n < m => z = n => 0 < m < z after swap
* case 1
* - c01 = 0 -> 0 > m
* - c12 = 1 -> m < n
* - c = 0   -> m < n < 0 => z = n => n < m < 0 after swap
* - c = 1   -> m < 0 < n => z = 0 => 0 < m < n after swap
            */
            z = (c == c12) ? 0 : count - 1;
            t = edges[z];
            edges[z] = edges[m];
            edges[m] = t;
        }
        
        // now edges[m] is the median-of-three
        // swap it to the beginning so it won't move around
        // NOTE(chan) : so edges[0] becomes the pivot in this division.
        t = edges[0];
        edges[0] = edges[m];
        edges[m] = t;
        
        i = 1;
        j = count - 1;
        for (;;)
        {
            for (;;++i) 
            {
                if(!(edges[i].y0 < edges[0].y0))
                    break;
            }
            
            for(;;--j)
            {
                if (!(edges[0].y0 < edges[j].y0))
                    break;
            }
            
            if (i >= j) 
                break;
            
            t = edges[i];
            edges[i] = edges[j];
            edges[j] = t;
            
            ++i;
            --j;
        }
        
        // recurse on smaller side, iterate on larger
        if (j < (count - i))
        {
            edges_sort_quick(edges, j);
            edges = edges + i;
            count = count - i;
        }
        else
        {
            edges_sort_quick(edges + i, count - i);
            count = j;
        }
    }
}

void edges_sort(Edge* edges, int count)
{
    edges_sort_quick(edges, count);
    edges_sort_insertion(edges, count);
}