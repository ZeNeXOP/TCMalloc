#pragma once
#include <stddef.h>



typedef struct {
    void* objects[TRANSFER_LIST_MAX_CAPACITY];
    int count;  //Number of valid pointers currently in the 'objects' array
} TransferListSlot;

int TransferList_FetchBatch(size_t size_class_idx, void** out_batch)