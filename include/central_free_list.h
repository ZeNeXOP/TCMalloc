#pragma once
#include <stddef.h>

void CentralFreeList_Init();

int CentralFreeList_FetchLargeBatch(size_t size_class_idx, void** out_batch, int n);

void CentralFreeList_ReturnBatch(void** batch, int n);