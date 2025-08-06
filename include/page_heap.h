#pragma once
#include "span.h"
#include <stddef.h>
#ifndef PAGE_HEAP_H
#define PAGE_HEAP_H
#define kPageSize 4096

typedef struct PageHeap
{
    Span large_spans_list;
} PageHeap;

void PageHeap_Init(PageHeap *self);
Span* PageHeap_Allocate(PageHeap *self, size_t num_pages);
void PageHeap_Deallocate(PageHeap *self, Span *span);
void* Malloc_Internal(size_t size);

#endif