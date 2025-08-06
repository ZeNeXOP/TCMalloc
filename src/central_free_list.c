#include "central_free_list.h"
#include "transfer_list.h"
#include "global.h"
#include "page_heap.h"
#include "utils.h"
#include "span.h"
#include "radix_tree.h" // Corrected header name
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <cstdint>
#include <cstring>

// Forward declaration for the helper function
static Span *AllocateNewSpan(size_t size_class_idx);
static int CentralFreeList_FetchLargeBatch_Unlocked(size_t size_class_idx, void **out_batch, int n);
static void CentralFreeList_ReturnBatch_Unlocked(size_t size_class_idx, void **batch, int n);

static TransferListSlot g_transfer_lists[NUM_SIZE_CLASSES];
static Span *g_cfl_span_lists[NUM_SIZE_CLASSES];
static pthread_mutex_t g_cfl_locks[NUM_SIZE_CLASSES];
PageHeap g_page_heap;
static pthread_once_t g_cfl_once_control = PTHREAD_ONCE_INIT;
pthread_mutex_t g_page_heap_lock;
pthread_mutex_t g_debug_print_lock;

static void CentralFreeList_Init_Internal()
{
    pthread_mutex_init(&g_page_heap_lock, NULL);
    pthread_mutex_init(&g_debug_print_lock, NULL);
    PageHeap_Init(&g_page_heap);
    PageMap_Module_Init();
    for (int i = 0; i < NUM_SIZE_CLASSES; ++i)
    {
        pthread_mutex_init(&g_cfl_locks[i], NULL);
        g_cfl_span_lists[i] = NULL;
        g_transfer_lists[i].count = 0;
    }
}

// To get a new span from PageHeap and set it up
//  ... existing code ...
static Span *AllocateNewSpan(size_t size_class_idx)
{
    size_t object_size = g_size_classes[size_class_idx];
    size_t num_objects = kPageSize / object_size;
    size_t num_pages_needed = (num_objects * object_size > kPageSize) ? 2 : 1; // Simplified; adjust as needed

    pthread_mutex_lock(&g_debug_print_lock);
    printf("[CFL:AllocateNewSpan] Acquiring page heap lock to allocate a new Span for class %zu.\n", size_class_idx);
    pthread_mutex_unlock(&g_debug_print_lock);

    pthread_mutex_lock(&g_page_heap_lock); // Protect entire back-end operation

    Span *new_span = PageHeap_Allocate(&g_page_heap, num_pages_needed);
    if (new_span == NULL)
    {
        pthread_mutex_unlock(&g_page_heap_lock);
        return NULL;
    }

    new_span->object_size = object_size;
    new_span->num_objects = num_objects;
    new_span->num_free_objects = num_objects;
    new_span->next_span = NULL;

    size_t bitmap_size = new_span->num_objects * sizeof(bool);
    new_span->object_bitmap = (bool *)Malloc_Internal(bitmap_size);
    if (new_span->object_bitmap == NULL)
    {
        // Handle allocation failure: clean up and return NULL
        // We can just return the span to the page heap, but for now, let's just fail
        // A more robust implementation would deallocate the span itself.
        pthread_mutex_unlock(&g_page_heap_lock);
        return NULL;
    }

    // Assume object_bitmap is allocated/init'd elsewhere
    memset(new_span->object_bitmap, 0, new_span->num_objects * sizeof(bool)); // All free

    // Register in PageMap (protected by the lock)
    uintptr_t page_id = (uintptr_t)new_span->start_address / kPageSize;
    for (size_t i = 0; i < new_span->num_pages; ++i)
    {
        PageMap_Set(page_id + i, new_span);
    }

    pthread_mutex_unlock(&g_page_heap_lock);
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[CFL:AllocateNewSpan] Releasing page heap lock. Span allocated at %p.\n", (new_span ? new_span->start_address : NULL));
    pthread_mutex_unlock(&g_debug_print_lock);
    return new_span;
}

void CentralFreeList_Init()
{
    pthread_once(&g_cfl_once_control, CentralFreeList_Init_Internal);
}

int MiddleTier_FetchFromCache(size_t size_class_idx, void **out_batch, int n)
{
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[MiddleTier:Fetch] Acquiring lock for class %zu to fetch %d objects.\n", size_class_idx, n);
    pthread_mutex_unlock(&g_debug_print_lock);
    // 1. Acquire the single, authoritative lock.
    pthread_mutex_lock(&g_cfl_locks[size_class_idx]);
    // 2. Try to get objects from the TransferList first.
    TransferListSlot *tl_list = &g_transfer_lists[size_class_idx];
    int num_fetched = TransferList_FetchBatch(tl_list, size_class_idx, out_batch, n);

    // 3. If the TL didn't have enough, use the CFL to refill the TL.
    if (num_fetched < n)
    {
        pthread_mutex_lock(&g_debug_print_lock);
        printf("[MiddleTier:Fetch] Transfer cache miss for class %zu. Trying to refill from Central Free List.\n", size_class_idx);
        pthread_mutex_unlock(&g_debug_print_lock);
        // How much space is left in the TL?
        int space_available = TRANSFER_LIST_CAPACITY - tl_list->count;
        if (space_available > 0)
        {
            // Fetch a large batch from the CFL's spans. This is a slow operation.
            int fetched_from_cfl = CentralFreeList_FetchLargeBatch_Unlocked(size_class_idx, &tl_list->objects[tl_list->count], space_available);
            tl_list->count += fetched_from_cfl;
        }

        // 3. Try one last time to get the remaining objects for the original request.
        int remaining_needed = n - num_fetched;
        int final_fetched = TransferList_FetchBatch(tl_list, size_class_idx, &out_batch[num_fetched], remaining_needed);
        num_fetched += final_fetched;
    }

    // 4. Release the lock.
    pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[MiddleTier:Fetch] Releasing lock for class %zu. Provided %d objects.\n", size_class_idx, num_fetched);
    pthread_mutex_unlock(&g_debug_print_lock);
    return num_fetched;
}

void MiddleTier_ReturnToCache(size_t size_class_idx, void **batch, int n)
{
    CentralFreeList_Init();
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[MiddleTier:Return] Acquiring lock for class %zu to return %d objects.\n", size_class_idx, n);
    pthread_mutex_unlock(&g_debug_print_lock);
    pthread_mutex_lock(&g_cfl_locks[size_class_idx]);

    // Try to place the returned objects in the TransferList first.
    TransferListSlot *tl_list = &g_transfer_lists[size_class_idx];
    int space_available = TRANSFER_LIST_CAPACITY - tl_list->count;
    int num_to_move = (n < space_available) ? n : space_available;

    if (num_to_move > 0)
    {
        memcpy(&tl_list->objects[tl_list->count], batch, num_to_move * sizeof(void *));
        tl_list->count += num_to_move;
    }

    // If there were leftovers that didn't fit in the TransferList,
    // return them directly to the CentralFreeList's Spans.
    int num_leftover = n - num_to_move;
    if (num_leftover > 0)
    {
        // We are already locked, so we can directly call the CFL's return logic.
        CentralFreeList_ReturnBatch_Unlocked(size_class_idx, &batch[num_to_move], num_leftover);
    }

    pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
    pthread_mutex_lock(&g_debug_print_lock);
    printf("[MiddleTier:Return] Releasing lock for class %zu.\n", size_class_idx);
    pthread_mutex_unlock(&g_debug_print_lock);
}

static int CentralFreeList_FetchLargeBatch_Unlocked(size_t size_class_idx, void **out_batch, int n)
{
    Span *current_span = g_cfl_span_lists[size_class_idx];
    while (current_span != NULL && current_span->num_free_objects == 0)
    {
        current_span = current_span->next_span;
    }

    if (current_span == NULL)
    {
        pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
        Span* new_span = AllocateNewSpan(size_class_idx);
        pthread_mutex_lock(&g_cfl_locks[size_class_idx]);
        if (new_span == NULL)
        {
            pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
            return 0; // Allocation failed
        }
        new_span->next_span = g_cfl_span_lists[size_class_idx];
        g_cfl_span_lists[size_class_idx] = new_span;
        current_span = new_span;
    }

    int fetched_count = 0;
    char *span_memory = (char *)current_span->start_address;
    for (int i = 0; i < current_span->num_objects && fetched_count < n; ++i)
    {
        if (current_span->object_bitmap[i] == false)
        {
            current_span->object_bitmap[i] = true;
            current_span->num_free_objects--;
            out_batch[fetched_count++] = (void *)(span_memory + (i * current_span->object_size));
        }
    }
    return fetched_count;
}

// NOTE: This implementation is simplified for clarity. A high-performance version
// would group pointers by Span first to reduce the number of lock/unlock cycles.
void CentralFreeList_ReturnBatch(void **batch, int n)
{
    CentralFreeList_Init();

    for (int i = 0; i < n; i++)
    {
        void *ptr = batch[i];
        if (ptr == NULL)
            continue;

        uintptr_t page_id = (uintptr_t)ptr / kPageSize;
        Span *span = PageMap_Get(page_id);
        if (span == NULL)
            continue; // Not our pointer

        size_t sc_idx = SizeMap_GetClass(span->object_size);
        pthread_mutex_lock(&g_cfl_locks[sc_idx]);

        // calculation of object's index within the span.
        uintptr_t offset = (uintptr_t)ptr - (uintptr_t)span->start_address;
        int object_idx = offset / span->object_size;

        if (span->object_bitmap[object_idx] == true)
        { // Check it was in use
            span->object_bitmap[object_idx] = false;
            span->num_free_objects++;
        }

        // TODO: Logic to check if span->num_free_objects == span->num_objects.
        // If so, the Span is fully free and can be returned to the PageHeap.

        pthread_mutex_unlock(&g_cfl_locks[sc_idx]);
    }
}

static void CentralFreeList_ReturnBatch_Unlocked(size_t size_class_idx, void **batch, int n)
{
    for (int i = 0; i < n; i++)
    {
        void *ptr = batch[i];
        if (ptr == NULL)
            continue;

        uintptr_t page_id = (uintptr_t)ptr / kPageSize;
        Span *span = PageMap_Get(page_id);
        if (span == NULL)
            continue;

        uintptr_t offset = (uintptr_t)ptr - (uintptr_t)span->start_address;
        int object_idx = offset / span->object_size;

        if (span->object_bitmap[object_idx] == true)
        {
            span->object_bitmap[object_idx] = false;
            span->num_free_objects++;
        }
    }
}