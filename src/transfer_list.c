#include "transfer_list.h"
#include "central_free_list.h"
#include "global.h"
#include <pthread.h>
#include <string.h>
#include <cstdio>



static TransferListSlot g_transfer_lists[NUM_SIZE_CLASSES]; //an array of arrays (transferlist)
static pthread_mutex_t g_tl_locks[NUM_SIZE_CLASSES]; //an array of mutexes
static pthread_once_t g_tl_once_control = PTHREAD_ONCE_INIT; //ensures a piece of code is run only one time in a multithreaded scenario


static void TransferList_Init_Internal() {
    for(int i = 0; i<NUM_SIZE_CLASSES; ++i){
        pthread_mutex_init(&g_tl_locks[i], NULL);
        g_transfer_lists[i].count = 0;
    }
}

void TransferList_Init(){
    pthread_once(&g_tl_once_control, TransferList_Init_Internal);
}//function used to call pthread_once so that the array of mutexes is initialized only once across
//all threads. pthread calls tliinternal,

int TransferList_FetchBatch(size_t size_class_idx, void** out_batch, int n){
    TransferList_Init();
    pthread_mutex_lock(&g_tl_locks[size_class_idx]);

    TransferListSlot* list = &g_transfer_lists[size_class_idx];

    if(list->count == 0){
        printf("TransferList: Empty, Refilling from CentralFreeList. \n");
        int space_available = TRANSFER_LIST_CAPACITY;
        // Fetch a new batch from the CFL and place it in our array.
        list->count = CentralFreeList_FetchLargeBatch(size_class_idx, list->objects, space_available);
        //Call CentralFreeList to get a new batch.
    }
    int num_to_move = (n < list->count) ? n : list->count;

    if (num_to_move > 0) {
        // Copy pointers from the end of our array to the output array.
        memcpy(out_batch, &list->objects[list->count - num_to_move], num_to_move * sizeof(void*));
        list->count -= num_to_move;
    }

    pthread_mutex_unlock(&g_tl_locks[size_class_idx]);
    return num_to_move;
}

void TransferList_ReturnBatch(size_t size_class_idx, void** in_batch, int n){
    TransferList_Init();
    pthread_mutex_lock(&g_tl_locks[size_class_idx]);

    TransferListSlot* list = &g_transfer_lists[size_class_idx];

    int space_available = TRANSFER_LIST_CAPACITY - list->count;
    int num_to_move = (n < space_available) ? n : space_available;

    if(num_to_move > 0){
        memcpy(&list->objects[list->count], in_batch, num_to_move*sizeof(void*));
        list->count += num_to_move;
    }

    int num_leftover = n - num_to_move;
    if(num_leftover > 0){
        CentralFreeList_ReturnBatch(&in_batch[num_to_move], num_leftover);
    }
    
    pthread_mutex_unlock(&g_tl_locks[size_class_idx]);
}