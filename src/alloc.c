#include "utils.h"
#include "tcmalloc.h"
#include "front_end.h"
#include "global.h"
#include <stdio.h>

void* MyMalloc(size_t size){
    size_t sc_idx = SizeMap_GetClass(size); // from utils.h -> utils.c

    //Limitation I can think of working on later, how do we handle large size allocations then?
    //we are limited to using 4096Bytes at one moment. its like giving change out of cash lol.
    //instead of using pageheap, we could remove objects from each array till requirement is fulfilled
    //like cash change. but then would it cause fragmentation?

    if (sc_idx != -1) {
        // Small object: handled by the frontend cache.
        return Frontend_Allocate(sc_idx);
    } else {
        // Large object: will be handled by the PageHeap directly.
        // TODO: Add call to PageHeap_Allocate for large objects.
        return NULL; // Placeholder for now.
    }
}

void MyFree(void* ptr){
    if(ptr == NULL) return;

    size_t sc_idx = SIZE_CLASS_16B;

    Frontend_Deallocate(ptr,sc_idx);
}