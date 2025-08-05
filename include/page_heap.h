#pragma once
#include "span.h"
#include <stddef.h>

#define kPageSize 4096

typedef struct {
    Span large_spans_list;
}PageHeap;


void PageMap_Module_Init(PageHeap8 self);

Span* PageHeap_Allocate(PageHeap* self, size_t num_pages);

void PageHeap_Deallocate(PageHeap* self, Span* span);