#pragma once
#include <stddef.h>
#include "global.h"

void* Frontend_Allocate(size_t size_class_idx);
void Frontend_Deallocate(void* ptr, size_t size);

//For testing purposes
void Frontend_Init_Test_Cache(size_t size_class_idx, size_t object_size, size_t count);