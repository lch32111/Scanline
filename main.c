/*
* 2024.02.02
* Reference 
* - https://github.com/nothings/stb/blob/master/stb_truetype.h
* - https://github.com/ssloy/tinyrenderer
* I write these codes to study the scan line fill algorithm for font rendering.
* I copy the stb_truetype.h code to understand the scanline fill algorithm.
*/

/* Scanline Conversion Algorithm from https://nothings.org/gamedev/rasterize/
* 1. Gather all of the directed edges into an array of edges
* 2. Sort them by their topmost vertex
* 3. Move a line down the polygon a scanline at a time, anf for each scanline:
*     - 1) Add to the "active edge list" any edges which have an uppermost vertex before this scanline, 
*       and store their x intersection with this scanline 
*       (because the edge list has been sorted by topmost y, 
*       edges from the edge list are always added in order)
*     - 2) Remove from the active edge list any edge whose lowestmost vertex is above this scaline
*     - 3) Sort the active edge list by their x intersection (incrementally, as it doesn't change much from scanline to scanline)
*     - 4) Iterate the active egde list from left to right, counting the crossing-number, and filling pixels as filled edges start and end
*     - 5) Advance every active edge to the next scanline by computing their next x intersection (which just requires adding a constant dx/dy associated with the edge)
*/

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include "heap.c"
#include "canvas.c"
#include "edge.c"
#include "rasterize1.c"

int main()
{
    const char* file_name = "scanline.png";
    Canvas* canvas = canvas_create(256, 256);
    
    int x_diff = 28;
    Vec2 vertices[] = 
    {
        {128, 20},
        {128 - x_diff, 60},
        {128 - x_diff * 2, 60},
        {128 - x_diff, 100},
        {128 - x_diff * 2, 140},
        {128, 100},
        {128 + x_diff * 2, 140},
        {128 + x_diff, 100},
        {128 + x_diff * 2, 60},
        {128 + x_diff, 60},
    };
    /*Vec2 vertices[] = 
    {
        {128, 20},
        {72, 140},
        {156, 140}
    };*/
    
    Polygon polygon = 
    {
        .count = ARRAY_COUNT(vertices),
        .vertices = vertices
    };
    float scale_x = 1.f;
    float scale_y = 1.f;
    float shift_x = 0.f;
    float shift_y = 0.f;
    int invert = 0;
    int vsubsample = 1;
    
    // Algorithm 1
    int edge_count = 0;
    Edge* edges = edges_alloc_for_raster_from_polygon(&polygon, scale_x, scale_y, shift_x, shift_y, invert, vsubsample, &edge_count);
    
    for(int i = 0; i < edge_count; ++i)
    {
        Edge* e = edges + i;
        printf("(%f, %f) - (%f, %f) - %d\n", e->x0, e->y0, e->x1, e->y1, e->invert);
    }
    
    // Algorithm 2
    // NOTE(chan) : sort the edges by edge->y0.
    // sort the edges by their highest point (should snap to integer, and then by x)
    edges_sort(edges, edge_count);
    
    // canvas_fill_edges(canvas, edges, edge_count, CANVAS_C_BLUE); // to see the edges are correct
    
    // Algorithm 3
    canvas_rasterize1_sorted_edges(canvas, edges, edge_count, vsubsample);
    
    edges_free(edges);
    
    canvas_save(canvas, "scanline.png");
    canvas_destroy(canvas);
    
    return 0;
}