#include "central_free_list.h"
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
#include "transfer_list.h"

// Forward declaration for the helper function
static Span* AllocateNewSpan(size_t size_class_idx);
static int CentralFreeList_FetchLargeBatch_Unlocked(size_t size_class_idx, void** out_batch, int n);


static TransferListSlot g_transfer_lists[NUM_SIZE_CLASSES];
static Span* g_cfl_span_lists[NUM_SIZE_CLASSES];
static pthread_mutex_t g_cfl_locks[NUM_SIZE_CLASSES];
static PageHeap g_page_heap;
static pthread_once_t g_cfl_once_control = PTHREAD_ONCE_INIT;

static void CentralFreeList_Init_Internal() {
    PageHeap_Init(&g_page_heap);
    PageMap_Module_Init();
    for(int i = 0; i < NUM_SIZE_CLASSES; ++i){
        pthread_mutex_init(&g_cfl_locks[i], NULL);
        g_cfl_span_lists[i] = NULL;
    }
}

//To get a new span from PageHeap and set it up
static Span* AllocateNewSpan(size_t size_class_idx){
    Span* span = PageHeap_Allocate(&g_page_heap, 1);
    if (!span) { return NULL; }

    span->object_size = SizeMap_GetSize(size_class_idx);
    span->num_objects = kPageSize / span->object_size;
    span->num_free_objects = span->num_objects;

    //Allocate and initialize the bitmap
    span->object_bitmap = (bool*)Malloc_Internal(sizeof(bool) * span->num_objects);
    if (!span->object_bitmap){
        //Return the span to the pageHeap
        PageHeap_Deallocate(&g_page_heap, span);
        return NULL;
    }
    for(int i = 0; i < span->num_objects; ++i){
        span->object_bitmap[i] = false; //false means free
    }

    uintptr_t start_page_id = (uintptr_t)span->start_address / kPageSize;
    for(uintptr_t i = 0; i < span->num_pages; ++i){
        PageMap_Set(start_page_id + i, span);
    }
    
    //Add the new span to the front of the CFL's list for this size class
    span->prev_span = NULL;
    span->next_span = g_cfl_span_lists[size_class_idx];
    if(g_cfl_span_lists[size_class_idx] != NULL){
        g_cfl_span_lists[size_class_idx]->prev_span = span;
    }
    g_cfl_span_lists[size_class_idx] = span;
    return span;
}

void CentralFreeList_Init(){
    pthread_once(&g_cfl_once_control, CentralFreeList_Init_Internal);
}

int MiddleTier_FetchFromCache(size_t size_class_idx, void** out_batch, int n) {
    // 1. Acquire the single, authoritative lock.
    pthread_mutex_lock(&g_cfl_locks[size_class_idx]);
    // 2. Try to get objects from the TransferList first.
    TransferListSlot* tl_list = &g_transfer_lists[size_class_idx];
    int num_fetched = TransferList_FetchBatch(tl_list, size_class_idx, out_batch, n);

    // 3. If the TL didn't have enough, use the CFL to refill the TL.
    if (num_fetched < n) {
        // How much space is left in the TL?
        int space_available = TRANSFER_LIST_CAPACITY - tl_list->count;
        if (space_available > 0) {
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
    return num_fetched;
}

static int CentralFreeList_FetchLargeBatch_Unlocked(size_t size_class_idx, void** out_batch, int n) {
    Span* current_span = g_cfl_span_lists[size_class_idx];
    while (current_span != NULL && current_span->num_free_objects == 0) {
        current_span = current_span->next_span;
    }

    if (current_span == NULL) {
        current_span = AllocateNewSpan(size_class_idx);
        if (current_span == NULL) {
            return 0; // Allocation failed
        }
    }

    int fetched_count = 0;
    char* span_memory = (char*)current_span->start_address;
    for (int i = 0; i < current_span->num_objects && fetched_count < n; ++i) {
        if (current_span->object_bitmap[i] == false) {
            current_span->object_bitmap[i] = true;
            current_span->num_free_objects--;
            out_batch[fetched_count++] = (void*)(span_memory + (i * current_span->object_size));
        }
    }
    return fetched_count;
}

// NOTE: This implementation is simplified for clarity. A high-performance version 
// would group pointers by Span first to reduce the number of lock/unlock cycles.
void CentralFreeList_ReturnBatch(void** batch, int n){
    for(int i = 0; i < n; i++){
        void* ptr = batch[i];
        if (ptr == NULL) continue;
        
        uintptr_t page_id = (uintptr_t)ptr / kPageSize;
        Span* span = PageMap_Get(page_id);
        if (span == NULL) continue; // Not our pointer

        size_t sc_idx = SizeMap_GetClass(span->object_size);
        pthread_mutex_lock(&g_cfl_locks[sc_idx]);
        
        //calculation of object's index within the span.
        uintptr_t offset = (uintptr_t)ptr - (uintptr_t)span->start_address;
        int object_idx = offset / span->object_size;

        if (span->object_bitmap[object_idx] == true) { // Check it was in use
            span->object_bitmap[object_idx] = false;
            span->num_free_objects++;
        }
        
        // TODO: Logic to check if span->num_free_objects == span->num_objects.
        // If so, the Span is fully free and can be returned to the PageHeap.

        pthread_mutex_unlock(&g_cfl_locks[sc_idx]);
    }
}