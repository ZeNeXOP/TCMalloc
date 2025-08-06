#include "front_end.h"
#include "global.h"
#include "transfer_list.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <cstring>
#include "central_free_list.h"
#define STACK_SIZE 64

typedef struct
{
    void *stack[NUM_SIZE_CLASSES][STACK_SIZE];
    int top[NUM_SIZE_CLASSES];
} PerThreadCache;

static thread_local PerThreadCache g_thread_cache = {.top = {0}}; // Array of pointers.

// static void ReturnBatchToTransferList(size_t size_class_idx){Logic to return half the stack to the Transfer List.}

static void RefillCache(size_t size_class_idx)
{
    PerThreadCache *cache = &g_thread_cache;

    const int batch_size_to_fetch = STACK_SIZE / 2;

    pthread_mutex_lock(&g_debug_print_lock);
    printf("[FrontEnd:RefillCache] Thread cache for class %zu is empty. Requesting %d objects from Middle Tier.\n", size_class_idx, batch_size_to_fetch);
    pthread_mutex_unlock(&g_debug_print_lock);

    void *batch_array[batch_size_to_fetch];
    int num_fetched = MiddleTier_FetchFromCache(size_class_idx, batch_array, batch_size_to_fetch);

    if (num_fetched > 0)
    {
        memcpy(&cache->stack[size_class_idx][0], batch_array, num_fetched * sizeof(void *));
        cache->top[size_class_idx] = num_fetched;
    }
    cache = &g_thread_cache; // Get cache again to be safe
    printf("Frontend: Refill complete. Fetched %d objects. New top is %d.\n", num_fetched, cache->top[size_class_idx]);
}

void *Frontend_Allocate(size_t size_class_idx)
{
    PerThreadCache *cache = &g_thread_cache;

    if (cache->top[size_class_idx] == 0)
    {
        printf("Frontend: Thread Cache Empty Requesting Transfer List for batch of objects for size class %zu. \n", size_class_idx);
        RefillCache(size_class_idx);
        printf("Objects Received: %d \n", cache->top[size_class_idx] + 1);
        if (cache->top[size_class_idx] == 0)
        {
            printf("Error, Objects not received. \n");
            return NULL;
        }
    }
    void* ptr = cache->stack[size_class_idx][--cache->top[size_class_idx]];
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[FrontEnd:Allocate] Cache hit for class %zu. Returning pointer %p.\n", size_class_idx, ptr);
    pthread_mutex_unlock(&g_debug_print_lock);
    return ptr;
}

void Frontend_Deallocate(void *ptr, size_t size_class_idx)
{
    PerThreadCache *cache = &g_thread_cache;

    if (cache->top[size_class_idx] >= STACK_SIZE)
    {
        pthread_mutex_lock(&g_debug_print_lock);
        printf("[FrontEnd:Deallocate] Thread cache for class %zu is full. Returning a batch to Middle Tier.\n", size_class_idx);
        pthread_mutex_unlock(&g_debug_print_lock);

        const int num_to_return = STACK_SIZE / 2;
        void **batch_to_return = &cache->stack[size_class_idx][0];

        MiddleTier_ReturnToCache(size_class_idx, batch_to_return, num_to_return);

        int remaining = cache->top[size_class_idx] - num_to_return;
        memmove(&cache->stack[size_class_idx][0], &cache->stack[size_class_idx][num_to_return], remaining * sizeof(void *));
        cache->top[size_class_idx] = remaining;
    }
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[FrontEnd:Deallocate] Returning pointer %p to thread cache for class %zu.\n", ptr, size_class_idx);
    pthread_mutex_unlock(&g_debug_print_lock);
    cache->stack[size_class_idx][cache->top[size_class_idx]++] = ptr;
}

// the functions where transfer list interacts with frontend are
// ReturnBatchToTransferList and TransferListFetchBatch.
