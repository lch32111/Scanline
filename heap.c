#include "def.h"

/*
* NOTE(chan) : This heap implementation only consider one size element.
*              So, you can't use this heap with different sizes
*/
static void* heap_alloc(Heap* h, size_t size)
{
    if (h->first_free)
    {
        void* p = h->first_free;
        h->first_free = *(void**)p;
        return p;
    }
    else
    {
        if (h->num_remaining_in_head_chunk == 0)
        {
            int count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
            HeapChunk* c = (HeapChunk*)malloc(sizeof(HeapChunk) + size * count);
            if(c == NULL)
                return NULL;
            
            c->next = h->head;
            h->head = c;
            h->num_remaining_in_head_chunk = count;
        }
        --(h->num_remaining_in_head_chunk);
        return ((char*)(h->head)) + sizeof(HeapChunk) + size * h->num_remaining_in_head_chunk;
    }
}

/*
* If you call heap_free two times, then
* *(void**)p = NULL; h->first_free = first_p;
* *(void**)p = first_p; h->first_Free = second_p;
* Then you call heap_alloc two times, then
* void* p = second_p; h->first_free = first_p;
* Therefore, it's just storing the previous freed pointer.
*/
static void heap_free(Heap* h, void* p)
{
    *(void**)p = h->first_free;
    h->first_free = p;
}

static void heap_cleanup(Heap* h)
{
    HeapChunk* c = h->head;
    while(c)
    {
        HeapChunk* n = c->next;
        free(c);
        c = n;
    }
}