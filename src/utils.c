#include "utils.h"
#include <stdio.h>

size_t SizeMap_GetClass(size_t size){
    if(size <= 8) return SIZE_CLASS_8B;
    if(size <= 16) return SIZE_CLASS_16B;
    if(size <= 32) return SIZE_CLASS_32B;
    if(size <= 64) return SIZE_CLASS_64B;
    if(size <= 128) return SIZE_CLASS_128B;
    if(size <= 256) return SIZE_CLASS_256B;
    if(size <= 512) return SIZE_CLASS_512B;
    if(size <= 1024) return SIZE_CLASS_1024B;
    if(size <= 2048) return SIZE_CLASS_2048B;
    if(size <= 4096) return SIZE_CLASS_4096B;
    if(size <= 8192) return SIZE_CLASS_8192B;
    return -1;
}

static const size_t g_size_class_sizes[] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

size_t SizeMap_GetSize(size_t size_class_idx) {

    printf("--- DEBUG: Inside SizeMap_GetSize with index: %zu ---\n", size_class_idx);

    if (size_class_idx >= NUM_SIZE_CLASSES) {
        return -1; // Invalid index
    }
    size_t size = g_size_class_sizes[size_class_idx];
    printf("--- DEBUG: Returning size: %zu ---\n", size);
    return size;
}