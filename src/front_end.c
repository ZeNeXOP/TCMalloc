#include "front_end.h"
#include "global.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

static _Thread_local void* g_thread_cache[NUM_SIZE_CLASSES] = {NULL}; //Array of pointers.

static void RefillCache(size_t size_class_idx){
    printf("Thread is refilling cache for size class %zu\n",size_class_idx);
    TransferList_FetchBatch()
    //placeholder for now
}

void* Frontend_Allocate(size_t size_class_idx){
    if(g_thread_cache[size_class_idx] == NULL){
        RefillCache(size_class_idx);
        if(g_thread_cache[size_class_idx] == NULL){
            return NULL;
        }
    }

    void* result = g_thread_cache[size_class_idx];
    g_thread_cache[size_class_idx] = *((void**)result);

    return result;
}

void Frontend_Deallocate(void* ptr, size_t size_class_idx){
    //there is no edge case of overflow like stack would have, considering its a free list 
    //which takes no space.
    *((void**)ptr) = g_thread_cache[size_class_idx];

    g_thread_cache[size_class_idx] = ptr;
}





/*
void Frontend_Init_Test_Cache(size_t size_class_idx, size_t object_size, size_t count) {
    // 1. Allocate a buffer using the system's original malloc.
    // We use stdlib::malloc to bypass our own override.
    char* buffer = (char*)malloc(object_size * count);
    if (!buffer) return;

    // 2. Clear the existing cache for a clean test.
    g_thread_cache[size_class_idx] = NULL;
    
    // 3. Carve the buffer into a linked list of objects.
    for (size_t i = 0; i < count; ++i) {
        void* current_obj = buffer + (i * object_size);
        // Push each fake object onto the stack.
        Frontend_Deallocate(current_obj, size_class_idx);
    }
}
*/