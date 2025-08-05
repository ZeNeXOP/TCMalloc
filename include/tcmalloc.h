#pragma once
#include <stddef.h>

// This tells the C++ compiler how to correctly link these C functions.
#ifdef __cplusplus
extern "C" {
#endif

// Public API for our allocator
void* MyMalloc(size_t size);
void MyFree(void* ptr);
void PrintStats();

#ifdef __cplusplus
}
#endif