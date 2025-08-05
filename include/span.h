#pragma once
#include <stddef.h>
#include <stdbool.h> // For the bool type

// A Span represents a contiguous run of memory pages. It acts as a node
// in a doubly linked list and manages the small objects allocated within it.
typedef struct Span {
    // Pointers for linking Spans together in the CentralFreeList's
    // doubly linked list.
    struct Span* next_span;
    struct Span* prev_span;

    // Location and size of the memory block this Span manages.
    void* start_address;
    size_t num_pages;
    
    // --- Object Management Metadata ---
    // These fields are set by the CentralFreeList when it carves this Span up.
    size_t object_size;       // The size of each object in this Span.
    int num_objects;          // Total number of object slots this Span can hold.
    int num_free_objects;     // A quick counter for currently free objects.
    bool* object_bitmap;      // The detailed inventory sheet (true = in use, false = free).
    
} Span;