#include "page_heap.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

static Span* GetMemoryFromOS(size_t num_pages){
    size_t bytes_to_get = num_pages * kPageSize;
    printf("PageHeap: Request %zu bytes from OS via mmap. \n", bytes_to_get);

    void* ptr = mmap(NULL, bytes_to_get, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ptr == MAP_FAILED){
        perror("mmap failed");
        return NULL;
    }

    Span* span = (Span*)Malloc_Internal(sizeof(Span));
    if(span == NULL){
        munmap(ptr, bytes_to_get);
        return NULL;
    }

    span->start_address = ptr;
    span->num_pages = num_pages;

    return span;
}

void PageHeap_Init(PageHeap* self){
    self->large_spans_list.next_span = &self->large_spans_list;
    self->large_spans_list.prev_span = &self->large_spans_list;
}

Span* PageHeap_Allocate(PageHeap* self, size_t num_pages){
    return GetMemoryFromOS(num_pages);
}
void* Malloc_Internal(size_t size) {
    // For simplicity, we'll just request a whole page.
    // A real implementation would manage a dedicated metadata slab.
    return mmap(NULL, kPageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
void PageHeap_Deallocate(PageHeap* self, Span* span){
    printf("PageHeap: Deallocating Span starting at %p covering %zu pages.\n",
           span->start_address, span->num_pages);

    span->next_span = self->large_spans_list.next_span;
    span->prev_span = &self->large_spans_list;   
    self->large_spans_list.next_span->prev_span = span;
    self->large_spans_list.next_span = span;

}