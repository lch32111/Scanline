#ifndef __CANVAS_H__
#define __CANVAS_H__

#define ARRAY_COUNT(arr) sizeof((arr)) / sizeof((arr)[0])
#define IFLOOR(x) ((int)floor(x))

/*
* NOTE(chan) : Top-down, left-right canvas.
*/
typedef struct Canvas
{
    uint8_t* p;
    int w, h, comp;
} Canvas;

typedef struct CanvasColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} CanvasColor;

typedef struct Vec2
{
    float x, y;
} Vec2;

typedef struct Polygon
{
    int count;
    Vec2* vertices;
} Polygon;

typedef struct Edge
{
    float x0, y0;
    float x1, y1;
    int invert;
} Edge;

typedef struct ActiveEdge
{
    struct ActiveEdge* next;
    int x, dx;
    float ey;
    int direction;
} ActiveEdge;

typedef struct HeapChunk
{
    struct HeapChunk* next;
} HeapChunk;

typedef struct Heap
{
    HeapChunk* head;
    void* first_free;
    int num_remaining_in_head_chunk;
} Heap;

#endif