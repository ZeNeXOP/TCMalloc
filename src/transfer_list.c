#include "transfer_list.h"
#include "central_free_list.h"
#include "global.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

int TransferList_FetchBatch(TransferListSlot* list, size_t size_class_idx, void** out_batch, int n) {
    
    // Determine how many objects we can actually provide.
    int num_to_move = (n < list->count) ? n : list->count;

    if (num_to_move > 0) {
        // Copy pointers from the end of our array to the output array.
        memcpy(out_batch, &list->objects[list->count - num_to_move], num_to_move * sizeof(void*));
        list->count -= num_to_move;
    }
    
    return num_to_move;
}

void TransferList_ReturnBatch(TransferListSlot* list, size_t size_class_idx, void** in_batch, int n) {
    int space_available = TRANSFER_LIST_CAPACITY - list->count;
    int num_to_move = (n < space_available) ? n : space_available;

    if (num_to_move > 0) {
        memcpy(&list->objects[list->count], in_batch, num_to_move * sizeof(void*));
        list->count += num_to_move;
    }

    int num_leftover = n - num_to_move;
    if (num_leftover > 0) {
        CentralFreeList_ReturnBatch(&in_batch[num_to_move], num_leftover);
    }
}