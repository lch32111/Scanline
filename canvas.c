#include "def.h"

static CanvasColor CANVAS_C_RED = {255, 0, 0};
static CanvasColor CANVAS_C_GREEN = {0, 255, 0};
static CanvasColor CANVAS_C_BLUE = {0, 0, 255};

Canvas* canvas_create(int w, int h)
{
    int comp = 1; // fix the number of component as 3 for brevity.
    uint64_t alloc_size = sizeof(Canvas) + w * h * comp;
    Canvas* canvas = (Canvas*)malloc(alloc_size);
    canvas->p = (uint8_t*)((uint8_t*)canvas + sizeof(Canvas));
    canvas->w = w;
    canvas->h = h;
    canvas->comp = comp;
    
    memset(canvas->p, 0, canvas->w * canvas->h * canvas->comp);
    return canvas;
}

void canvas_destroy(Canvas* canvas)
{
    free(canvas);
}

void canvas_save(Canvas* canvas, const char* file_name)
{
    int result = stbi_write_png(file_name, canvas->w, canvas->h, canvas->comp, canvas->p, canvas->w * canvas->comp);
    
    if (result == 0)
        printf("Fail to save a canvas on %s\n", file_name);
}

void canvas_fill_color_rgb(Canvas* canvas, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* target = canvas->p + canvas->w * canvas->comp * y + canvas->comp * x;
    
    target[0] = r;
    target[1] = g;
    target[2] = b;
}

void canvas_fill_color(Canvas* canvas, int x, int y, CanvasColor color)
{
    uint8_t* target = canvas->p + canvas->w * canvas->comp * y + canvas->comp * x;
    
    target[0] = color.r;
    target[1] = color.g;
    target[2] = color.b;
}

// from https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm
// unoptimized version line filling.
void canvas_fill_line(Canvas* canvas, int x0, int y0, int x1, int y1, CanvasColor color)
{
    int temp, y;
    float t;
    bool steep = false;
    if(fabsf(x0 - x1) < fabsf(y0 - y1)) // steep line
    {
        temp = x0;
        x0 = y0;
        y0= temp;
        
        temp = x1;
        x1 = y1;
        y1 = temp;
        
        steep = true;
    }
    
    if(x0 > x1) // make it left-to-right
    {
        temp = x0;
        x0 = x1;
        x1 = temp;
        
        temp = y0;
        y0 = y1;
        y1 = temp;
    }
    
    for(temp = x0; temp <= x1; ++temp)
    {
        t = (temp - x0) / (float)(x1 - x0);
        y = y0 * (1.f - t) + y1 * t;
        
        if (steep)
            canvas_fill_color(canvas, y, temp, color);
        else
            canvas_fill_color(canvas, temp, y, color);
    }
}

void canvas_fill_edges(Canvas* canvas, Edge* edges, int edge_count, CanvasColor color)
{
    for(int ei = 0; ei < edge_count; ++ei)
    {
        Edge* e = edges + ei;
        canvas_fill_line(canvas, e->x0, e->y0, e->x1, e->y1, color);
    }
}