#include "radix_tree.h"
#include "page_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


static PageMap g_pagemap;
static int g_pagemap_initialized = 0;

void PageMap_Module_Init(){
    if (g_pagemap_initialized) return;


    g_pagemap.children = (PageMapNode_L2**)calloc(PAGEMAP_L1_SIZE, sizeof(PageMapNode_L2*));
    if(g_pagemap.children == NULL){
        perror("Failed to initialize PageMap root");
        exit(1);
    }
    g_pagemap_initialized = 1;
}

//Sets the span for a given page ID
void PageMap_Set(uintptr_t page_id, Span* span){
    if(!g_pagemap_initialized) PageMap_Module_Init();

    //Extraction of indices from the page_id using bitwise operations.
    const uintptr_t l1_idx = (page_id >> (PAGEMAP_L2_BITS + PAGEMAP_L3_BITS));
    const uintptr_t l2_idx = (page_id >> PAGEMAP_L3_BITS) & (PAGEMAP_L2_SIZE - 1);
    const uintptr_t l3_idx = page_id & (PAGEMAP_L3_SIZE - 1);

    //Traverse Level 1 -> Level 2
    if(g_pagemap.children[l1_idx] == NULL){
        //Allocating L2 node if it doesnt exist.
        g_pagemap.children[l1_idx] = (PageMapNode_L2*)calloc(1, sizeof(PageMapNode_L2));
        if(g_pagemap.children[l1_idx] == NULL) return;
    }

    //Traverse Level 2 -> Level 3
    if (l2_node->children[l2_idx] == NULL){
        //Allocating L3 node if it doesnt exist.
        l2_node->children[l2_idx] = (PageMapNode_L3*)calloc(1, sizeof(PageMapNode_L3));
        if(l2_node->children[l2_idx] == NULL) return;
    }

    //Set value at the leaf
    PageMapNode_L3* l3_node = l2_node->children[l2_idx];
    l3_node->values[l3_idx] = span;
}

//Gets the span for a given page id
Span* PageMap_Get(PageMap* self, uintptr_t page_id){
    if (!g_pagemap_initialized) return NULL;

    //Extraction of indices from the page_id using bitwise operations.
    const uintptr_t l1_idx = (page_id >> (PAGEMAP_L2_BITS + PAGEMAP_L3_BITS));
    const uintptr_t l2_idx = (page_id >> PAGEMAP_L3_BITS) & (PAGEMAP_L2_SIZE - 1);
    const uintptr_t l3_idx = page_id & (PAGEMAP_L3_SIZE - 1);

    //Traverse Level 1 -> Level 2
    PageMapNode_L2* l2_node = g_pagemap.children[l1_idx];
    if (l2_node == NULL) {
        return NULL; // No entry
    }

    //Traverse Level 2 -> Level 3
    PageMapNode_L3* l3_node = l2_node->children[l2_idx];
    if (l3_node == NULL) {
        return NULL; // No entry
    }

    //Return the value from the leaf.
    return l3_node->values[l3_idx];
    
}