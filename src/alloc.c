#include "utils.h"
#include "tcmalloc.h"
#include "front_end.h"
#include "global.h"
#include "page_heap.h"
#include "radix_tree.h"
#include <stdio.h>
#include <cstdint>
#include <pthread.h>

void* MyMalloc(size_t size){
    size_t sc_idx = SizeMap_GetClass(size); // from utils.h -> utils.c
    pthread_mutex_lock(&g_debug_print_lock);
    printf("Memory requested: %zu \n", sc_idx);
    pthread_mutex_unlock(&g_debug_print_lock);

    //Limitation I can think of working on later, how do we handle large size allocations then?
    //we are limited to using 4096Bytes at one moment. its like giving change out of cash lol.
    //instead of using pageheap, we could remove objects from each array till requirement is fulfilled
    //like cash change. but then would it cause fragmentation?

    if (sc_idx != -1) {
        // Small object: handled by the frontend cache.
        pthread_mutex_lock(&g_debug_print_lock);
        printf("Searching for size class object in Frontend Thread Cache \n");
        pthread_mutex_unlock(&g_debug_print_lock);
        return Frontend_Allocate(sc_idx);
    } else {
        // Large object: will be handled by the PageHeap directly.
        // TODO: Add call to PageHeap_Allocate for large objects.
        return NULL; // Placeholder for now.
    }
}



void MyFree(void* ptr){
    if(ptr == NULL) return;

    pthread_mutex_lock(&g_page_heap_lock);
    uintptr_t page_id = (uintptr_t)ptr / kPageSize;
    Span* span = PageMap_Get(page_id);
    pthread_mutex_unlock(&g_page_heap_lock);

    if(span == NULL) return;

    size_t sc_idx = SizeMap_GetClass(span->object_size);

    Frontend_Deallocate(ptr,sc_idx);
}