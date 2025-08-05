#include "central_free_list.h"
#include "global.h"
#include "page_heap.h"
#include "utils.h"
#include "span.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "radix_tree.h"
#include <stdbool.h>
#include <cstdint>

static Span* AllocateNewSpan(size_t size_class_idx);

static Span* g_cfl_span_lists[NUM_SIZE_CLASSES];
static pthread_mutex_t g_cfl_locks[NUM_SIZE_CLASSES];
static PageHeap g_page_heap;
static pthread_once_t g_cfl_once_control = PTHREAD_ONCE_INIT;

static void CentralFreeList_Init_Internal() {
    PageHeap_Init(&g_page_heap);
    for(int i = 0; i < NUM_SIZE_CLASSES; ++i){
        pthread_mutex_init(&g_cfl_locks[i], NULL);
        g_cfl_span_lists[i] = NULL;
    }
}

//To get a new span from PageHeap and set it up
static Span* AllocateNewSpan(size_t size_class_idx){
    Span* span = PageHeap_Allocate(&g_page_heap, 1);
    if (!span) return NULL;

    span->object_size = SizeMap_GetClass(size_class_idx);
    span->num_objects = kPageSize/span->object_size;
    span->num_free_objects = span->num_objects;

    //Allocate and initialize the bitmap
    span->object_bitmap = (bool*)malloc(sizeof(bool) * span->num_objects);
    if (!span->object_bitmap){
        //Handle allocation failure for bitmap itself
        //Return the span to the pageHeap
        return NULL;
    }
    for(int i = 0; i< span->num_objects; ++i){
        span->object_bitmap[i] = false; //false means free
    }

    uintptr_t start_page_id = (uintptr_t)span->start_address / kPageSize;
    for(uintptr_t i = 0; i < span->num_pages; ++i){
        PageMap_Set(start_page_id + i, span);
    }
    //Add the new span to the front of the CFL's list for this size class
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

int CentralFreeList_FetchLargeBatch(size_t size_class_idx, void** out_batch, int n){
    CentralFreeList_Init();
    pthread_mutex_lock(&g_cfl_locks[size_class_idx]);

    Span* current_span = g_cfl_span_lists[size_class_idx];
    while(current_span != NULL && current_span->num_free_objects == 0){
        current_span = current_span->next_span;
    }

    if (current_span == NULL){
        current_span = AllocateNewSpan(size_class_idx);
        if(current_span == NULL){
            pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
            return 0;
        }
    }

    int fetched_count = 0;
    char* span_memory = (char*)current_span->start_address;
    for(int i = 0; i<current_span->num_objects && fetched_count<n; ++i){
        if(current_span->object_bitmap[i] == false){
            current_span->object_bitmap[i] = true;
            current_span->num_free_objects--;
            out_batch[fetched_count++] = (void*)(span_memory + (i*current_span->object_size));
        }
    }

    pthread_mutex_unlock(&g_cfl_locks[size_class_idx]);
    return fetched_count;
}

void CentralFreeList_ReturnBatch(void** batch, int n){
    for(int i = 0; i<n; i++){
        void* ptr = batch[i];
        uintptr_t page_id = (uintptr_t)ptr / kPageSize;
        Span* span = PageMap_Get(page_id);

        pthread_mutex_lock(&g_cfl_locks[SizeMap_GetClass(span->object_size)]);
        
        //calculation of object's index within the span.
        uintptr_t offset = (uintptr_t)ptr - (uintptr_t)span->start_address;
        int object_idx = offset/span->object_size;

        if (span->object_bitmap[object_idx] == true) {
            span->object_bitmap[object_idx] = false;
            span->num_free_objects++;
        }

        pthread_mutex_unlock(&g_cfl_locks[SizeMap_GetClass(span->object_size)]);
    }
}