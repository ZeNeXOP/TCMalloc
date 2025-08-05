#include "front_end.h"
#include "global.h"
#include "transfer_list.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstring>
#define STACK_SIZE 64

typedef struct{
    void* stack[NUM_SIZE_CLASSES][STACK_SIZE];
    int top[NUM_SIZE_CLASSES];
}PerThreadCache;

static thread_local PerThreadCache g_thread_cache = {.top = {0}}; //Array of pointers.

//static void ReturnBatchToTransferList(size_t size_class_idx){Logic to return half the stack to the Transfer List.}

static void RefillCache(size_t size_class_idx){
    PerThreadCache* cache = &g_thread_cache;
    printf("Thread cache empty. Refilling from Transfer List for size class %zu. \n", size_class_idx);

    const int batch_size_to_fetch = STACK_SIZE/2;
    void* batch_array[batch_size_to_fetch];

    int num_fetched = TransferList_FetchBatch(size_class_idx, batch_array, batch_size_to_fetch);

    if(num_fetched > 0){
        memcpy(&cache->stack[size_class_idx][0], batch_array, num_fetched * sizeof(void*));
        cache->top[size_class_idx] = num_fetched;
    }
}

void* Frontend_Allocate(size_t size_class_idx){
    PerThreadCache* cache = &g_thread_cache;

    if(cache->top[size_class_idx] == 0){
        RefillCache(size_class_idx);
        if(cache->top[size_class_idx] == 0){
            return NULL;
        }
    }
    return cache->stack[size_class_idx][--cache->top[size_class_idx]];
}

void Frontend_Deallocate(void* ptr, size_t size_class_idx){
    PerThreadCache* cache = &g_thread_cache;

    if (cache->top[size_class_idx] >= STACK_SIZE){
        const int num_to_return = STACK_SIZE/2;
        void** batch_to_return = &cache->stack[size_class_idx][0];

        TransferList_ReturnBatch(size_class_idx, batch_to_return, num_to_return);

        int remaining = cache->top[size_class_idx] - num_to_return;
        memmove(&cache->stack[size_class_idx][0], &cache->stack[size_class_idx][num_to_return], remaining * sizeof(void*));
        cache->top[size_class_idx] = remaining;
    }

    cache->stack[size_class_idx][cache->top[size_class_idx]++] = ptr;
}


//the functions where transfer list interacts with frontend are 
//ReturnBatchToTransferList and TransferListFetchBatch.
