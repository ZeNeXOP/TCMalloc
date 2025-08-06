#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <mutex>

// --- C++ Linkage to C Functions ---
// This block tells the C++ compiler that our MyMalloc and MyFree functions
// are written in C and to link them accordingly.
extern "C" {
    #include "tcmalloc.h"
}

const int NUM_THREADS = 4;
const int ALLOCATIONS_PER_THREAD = 10;
const int LOOP_ITERATIONS = 5;

static std::mutex g_cout_mutex;

// This is the function that each thread will execute.
void WorkerThread(int thread_id) {
    // Each thread gets its own random number generator.
    std::mt19937 gen(thread_id);
    std::uniform_int_distribution<> size_dist(1, 1024); // Allocate sizes from 1 to 1024 bytes

    for (int i = 0; i < LOOP_ITERATIONS; ++i) {
        std::vector<void*> allocations;
        allocations.reserve(ALLOCATIONS_PER_THREAD);

        // --- Allocation Phase ---
        for (int j = 0; j < ALLOCATIONS_PER_THREAD; ++j) {
            allocations.push_back(MyMalloc(size_dist(gen)));
        }

        // --- Deallocation Phase ---
        for (void* ptr : allocations) {
            MyFree(ptr);
        }
    }
    std::lock_guard<std::mutex> guard(g_cout_mutex);
    std::cout << "Thread " << thread_id << " finished." << std::endl;
}

int main() {
    std::cout << "Starting multithreaded stress test with " << NUM_THREADS << " threads..." << std::endl;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    // Launch all threads.
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(WorkerThread, i + 1);
    }

    // Wait for all threads to complete.
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All threads finished." << std::endl;

    return 0;
}