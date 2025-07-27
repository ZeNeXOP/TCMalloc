#include "tcmalloc.h"
#include "front_end.h"
#include "global.h"
#include <stdio.h>

int main() {
    printf("--- Testing Frontend with Pre-filled Cache ---\n");

    // --- Test Setup ---
    // We will test the 16-byte size class.
    const size_t test_object_size = 16;
    const size_t object_count_to_create = 5;
    const size_t test_size_class_index = SIZE_CLASS_16B;

    // Pre-populate the cache with 5 objects of 16 bytes.
    Frontend_Init_Test_Cache(test_size_class_index, test_object_size, object_count_to_create);
    printf("Initialized cache for %zu-byte objects with %zu items.\n\n", test_object_size, object_count_to_create);

    // --- Allocation Phase ---
    void* pointers[object_count_to_create];
    printf("Allocating %zu objects...\n", object_count_to_create);
    for (size_t i = 0; i < object_count_to_create; ++i) {
        // Request a size that will map to our 16-byte class (e.g., 12 bytes).
        pointers[i] = MyMalloc(12);
        if (pointers[i]) {
            printf("  - Allocation %zu successful. Pointer: %p\n", i + 1, pointers[i]);
        } else {
            printf("  - ERROR: Allocation %zu failed!\n", i + 1);
        }
    }

    // --- Deallocation Phase ---
    printf("\nDeallocating %zu objects...\n", object_count_to_create);
    for (size_t i = 0; i < object_count_to_create; ++i) {
        MyFree(pointers[i]);
    }
    printf("  - Deallocation calls complete.\n");

    // --- Verification Phase ---
    printf("\nVerifying deallocation by re-allocating...\n");
    void* reallocated_ptr = MyMalloc(16);
    if (reallocated_ptr) {
        printf("  - Re-allocation successful. Pointer: %p\n", reallocated_ptr);
        MyFree(reallocated_ptr);
    } else {
        printf("  - ERROR: Re-allocation failed!\n");
    }

    printf("\n--- Frontend Test Complete ---\n");
    return 0;
}