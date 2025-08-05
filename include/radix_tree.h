#pragma once 
#include "span.h"
#include <stddef.h>
#include <cstdint>

#define PAGEMAP_L3_BITS 12
#define PAGEMAP_L3_SIZE (1 << PAGEMAP_L3_BITS)
typedef struct {
    Span* values[PAGEMAP_L3_SIZE];
} PageMapNode_L3;

#define PAGEMAP_L2_BITS 12
#define PAGEMAP_L2_SIZE (1 << PAGEMAP_L2_BITS)
typedef struct {
    PageMapNode_L3* children[PAGEMAP_L2_SIZE];
} PageMapNode_L2;

#define PAGEMAP_L1_BITS 12
#define PAGEMAP_L1_SIZE (1 << PAGEMAP_L1_BITS)
typedef struct {
    PageMapNode_L2* children[PAGEMAP_L1_SIZE];
} PageMap;

extern PageMap g_pagemap;

void PageMap_Module_Init();
void PageMap_Set(uintptr_t page_id, Span* span);
Span* PageMap_Get(uintptr_t page_id);