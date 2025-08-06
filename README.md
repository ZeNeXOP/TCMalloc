# A High-Performance, Multi-Threaded Memory Allocator in C/C++
## *Inspired by Google's TCMalloc*

---

## Project Overview

This project is a **custom high-performance memory allocator** designed for **multi-threaded applications** on POSIX-compliant systems (e.g., Linux, WSL). It is inspired by the architectural principles of **Google's TCMalloc (Thread-Caching Malloc)**.

Its primary goals are:
- Minimize lock contention
- Scale efficiently with CPU cores
- Provide extremely fast allocation/deallocation paths for small objects

Compile: g++ test.cpp src/*.c -o my_program -Iinclude -pthread -std=c++17 -g
Execute: ./my_program

---

## Core Architectural Principles

- **Hierarchical Caching**  
  Three-tier memory hierarchy: *Front-End*, *Middle-End*, *Back-End*
  
- **Size-Classing**  
  Allocation requests are rounded up to fixed-size "size classes" (e.g., 8, 16, ..., 1024 bytes)

- **Thread-Centric Design**  
  Fast-path allocation is *entirely thread-local* — no locks or atomics

- **Batch Operations**  
  Memory is transferred between layers in *batches* to amortize locking overhead

---

## Detailed Architecture

### 1. Front-End: *Per-Thread Cache*

- **Purpose**: Ultra-fast, lock-free memory reuse  
- **Implementation**: `thread_local` caches (`PerThreadCache`) with an array of free lists, one per size class  
- **Concurrency**: Completely lock-free  
- **Operations**:
  - `MyMalloc(size)`: Pops from front-end freelist
  - `MyFree(ptr)`: Pushes to front-end freelist

---

### 2. Middle-End: *Transfer Cache + Central Free List*

- **Transfer Cache (`TransferListSlot`)**  
  - Small global buffer (per size class)  
  - Fast buffer for pushing/pulling batches  
  - Requires per-size-class lock  

- **Central Free List (`CentralFreeList`)**  
  - Holds the *true source of memory* (Spans)  
  - One list per size class  
  - Uses fine-grained locks (`g_cfl_locks[]`)

---

### 3. Back-End: *Page Heap*

- **Purpose**: Interface with OS to allocate large, aligned memory chunks  
- **Implementation**: Uses `mmap()` for allocation  
- **Concurrency**: Global mutex (`g_page_heap_lock`)  
- **Operations**:
  - Allocate large spans
  - Split spans into objects
  - Free back to OS via `munmap()` when unused

---

## Key Data Structures

### Span

- Represents a contiguous range of pages
- Serves only *one* size class
- Fields include:
  - Object size
  - Number of objects
  - Allocation bitmap
  - Pointer to memory range

---

### PageMap (Radix Tree)

- Maps raw object pointers to their corresponding `Span`
- Allows fast lookup in `MyFree(ptr)`
- Structured in 3 levels (L1, L2, L3), each indexing bits from a 48-bit virtual address

---

## Allocation & Deallocation Flow

### **MyMalloc(size)**

1. Map size to size class
2. Try `PerThreadCache` → fast path (no lock)
3. On miss:
   - Acquire lock on `TransferCache[class]`, try batch
4. If TC is empty:
   - Lock `CentralFreeList[class]`, get from spans
5. If CFL is empty:
   - Lock `PageHeap`, request new span using `mmap()`
6. Return object to caller

---

### **MyFree(ptr)**

1. Use `PageMap` to find associated Span
2. Determine size class from Span
3. Push to `PerThreadCache` (fast path)
4. If cache is full:
   - Create batch, push to `TransferCache`
   - If TC is full, flush to `CentralFreeList`

---

## Build & Testing

- **Languages**: `C`, `C++`
- **Platform**: Linux / WSL (POSIX)
- **Compiler**: `gcc` or `g++` with `-pthread`
- **Test Tool**: `test.cpp` simulates high concurrency

### Debugging Tools

| Tool | Purpose |
|------|---------|
| `gdb` | Debugging crashes, segfaults |
| `valgrind --tool=helgrind` | Detects race conditions and deadlocks |

---

## Features Implemented

- Thread-local caches for fast allocation
- Transfer caches and central free lists
- Radix tree-backed page-to-span mapping
- Batched memory movement
- Efficient, size-classed spans
- PageHeap layer using `mmap`
- Stress-tested for multi-threading

## 🏁 Conclusion

This project delivers a **functionally complete and performance-conscious memory allocator**, mirroring the internal architecture of TCMalloc. It is suitable for:

- Systems programming practice  
- Research into memory allocators  
- High-performance multi-threaded applications  

---
## Directory Structure

