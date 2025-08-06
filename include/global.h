#pragma once
#define NUM_SIZE_CLASSES 11 // 8B, 16B, 32B, 64B, 128B, 256B, 512B, 1024B, 2048B, 4096B, 8192B
#include <pthread.h>
#include <stddef.h>
#include "page_heap.h"
#ifndef GLOBAL_H
#define GLOBAL_H
extern const size_t g_size_classes[NUM_SIZE_CLASSES];
enum SizeClassIndex
{
    SIZE_CLASS_8B = 0,
    SIZE_CLASS_16B = 1,
    SIZE_CLASS_32B = 2,
    SIZE_CLASS_64B = 3,
    SIZE_CLASS_128B = 4,
    SIZE_CLASS_256B = 5,
    SIZE_CLASS_512B = 6,
    SIZE_CLASS_1024B = 7,
    SIZE_CLASS_2048B = 8,
    SIZE_CLASS_4096B = 9,
    SIZE_CLASS_8192B = 10
};

extern PageHeap g_page_heap;
extern pthread_mutex_t g_page_heap_lock;   // New: Protects PageHeap and PageMap
extern pthread_mutex_t g_debug_print_lock; // New: Protects debug printf/puts

#endif
