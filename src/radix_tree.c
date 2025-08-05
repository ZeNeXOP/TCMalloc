#include "radix_tree.h"
#include "page_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


PageMap g_pagemap;
static int g_pagemap_initialized = 0;

void PageMap_Module_Init(){
    if (g_pagemap_initialized) return;

    for (int i = 0; i < PAGEMAP_L1_SIZE; ++i) {
        g_pagemap.children[i] = NULL;
    }
    g_pagemap_initialized = 1;
}

//Sets the span for a given page ID
void PageMap_Set(uintptr_t page_id, Span* span){

    //Extraction of indices from the page_id using bitwise operations.
    const uintptr_t l1_idx = (page_id >> (PAGEMAP_L2_BITS + PAGEMAP_L3_BITS));
    const uintptr_t l2_idx = (page_id >> PAGEMAP_L3_BITS) & (PAGEMAP_L2_SIZE - 1);
    const uintptr_t l3_idx = page_id & (PAGEMAP_L3_SIZE - 1);

    //Traverse Level 1 -> Level 2
    if (g_pagemap.children[l1_idx] == NULL) {
        g_pagemap.children[l1_idx] = (PageMapNode_L2*)calloc(1, sizeof(PageMapNode_L2));
    }

    //Traverse Level 2 -> Level 3
    PageMapNode_L2* l2_node = g_pagemap.children[l1_idx];
    if (l2_node->children[l2_idx] == NULL) {
        l2_node->children[l2_idx] = (PageMapNode_L3*)calloc(1, sizeof(PageMapNode_L3));
    }

    //Set value at the leaf
    PageMapNode_L3* l3_node = l2_node->children[l2_idx];
    l3_node->values[l3_idx] = span;
}

//Gets the span for a given page id
Span* PageMap_Get(uintptr_t page_id){
    //Extraction of indices from the page_id using bitwise operations.
    const uintptr_t l1_idx = (page_id >> (PAGEMAP_L2_BITS + PAGEMAP_L3_BITS));
    const uintptr_t l2_idx = (page_id >> PAGEMAP_L3_BITS) & (PAGEMAP_L2_SIZE - 1);
    const uintptr_t l3_idx = page_id & (PAGEMAP_L3_SIZE - 1);

    //Traverse Level 1 -> Level 2
    PageMapNode_L2* l2_node = g_pagemap.children[l1_idx];
    if (l2_node == NULL) return NULL;

    //Traverse Level 2 -> Level 3
    PageMapNode_L3* l3_node = l2_node->children[l2_idx];
    if (l3_node == NULL) return NULL;

    //Return the value from the leaf.
    return l3_node->values[l3_idx];
    
}