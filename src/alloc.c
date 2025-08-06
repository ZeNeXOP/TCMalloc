#include "utils.h"
#include "tcmalloc.h"
#include "front_end.h"
#include "global.h"
#include "page_heap.h"
#include "radix_tree.h"
#include <stdio.h>
#include <cstdint>
#include <pthread.h>

void *MyMalloc(size_t size)
{
    size_t sc_idx = SizeMap_GetClass(size); // from utils.h -> utils.c
    pthread_mutex_lock(&g_debug_print_lock);
    printf("Memory requested: %zu \n", sc_idx);
    pthread_mutex_unlock(&g_debug_print_lock);

    // Limitation I can think of working on later, how do we handle large size allocations then?
    // we are limited to using 4096Bytes at one moment. its like giving change out of cash lol.
    // instead of using pageheap, we could remove objects from each array till requirement is fulfilled
    // like cash change. but then would it cause fragmentation?
    if (size == 0) {
        // Technically, C standard says malloc(0) can return NULL or a unique ptr.
        // We'll return NULL to keep it simple.
        pthread_mutex_lock(&g_debug_print_lock);
        printf("Request size is 0, returning NULL.\n");
        pthread_mutex_unlock(&g_debug_print_lock);
        return NULL;
    }


    if (sc_idx != -1)
    {
        // Small object: handled by the frontend cache.
        pthread_mutex_lock(&g_debug_print_lock);
        printf("Searching for size class object in Frontend Thread Cache \n");
        pthread_mutex_unlock(&g_debug_print_lock);
        return Frontend_Allocate(sc_idx);
    }
    else
    {
        pthread_mutex_lock(&g_debug_print_lock);
        printf("Size is too large for any class. Routing to Page Heap (TODO).\n");
        pthread_mutex_unlock(&g_debug_print_lock);
        // TODO: Add call to PageHeap_Allocate for large objects.
        return NULL; // Placeholder for now.
    }
}

void MyFree(void *ptr)
{
    pthread_mutex_lock(&g_debug_print_lock);
    printf("Received request to free pointer %p.\n", ptr);
    pthread_mutex_unlock(&g_debug_print_lock);
    if (ptr == NULL)
        return;

    pthread_mutex_lock(&g_page_heap_lock);
    uintptr_t page_id = (uintptr_t)ptr / kPageSize;
    Span *span = PageMap_Get(page_id);
    pthread_mutex_unlock(&g_page_heap_lock);

    if (span == NULL){
        pthread_mutex_lock(&g_debug_print_lock);
        printf("Pointer %p is not in any span. Ignoring free request.\n", ptr);
        pthread_mutex_unlock(&g_debug_print_lock);
        return;
    }

    size_t sc_idx = SizeMap_GetClass(span->object_size);
    pthread_mutex_lock(&g_debug_print_lock);
    printf("Pointer %p belongs to span for size class %zu. Routing to Front-End.\n", ptr, sc_idx);
    pthread_mutex_unlock(&g_debug_print_lock);

    Frontend_Deallocate(ptr, sc_idx);
}