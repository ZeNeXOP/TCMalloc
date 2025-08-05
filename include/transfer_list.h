#pragma once
#include <stddef.h>

#define TRANSFER_LIST_CAPACITY 64

typedef struct {
    void* objects[TRANSFER_LIST_MAX_CAPACITY];
    int count;  //Number of valid pointers currently in the 'objects' array
} TransferListSlot;

void TransferList_Init(); //Initializes the module setting up mutexes. 

int TransferList_FetchBatch(size_t size_class_idx, void** out_batch, int n);
//Fills the outbatch array with up to 'n' pointers from the Transfer List.
//Returns the actual number of pointers fetched.


void TransferList_ReturnBatch(size_t size_class_idx, void** in_batch, int n);
//Returns a batch of 'n' pointers from 'in_batch' to the Transfer List.