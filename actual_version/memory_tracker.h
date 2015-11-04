#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

//comment this out to not track memory usage
#define USE_MEMORY_TRACKING

//The memory tracker is designed to track memory usage of the entire program and
//particular algorithms. alloc and dealloc can be used in place of malloc and free.

#ifdef USE_MEMORY_TRACKING

#include <stdint.h>

//allocates a named pointer, which can be printed out later if not deallocated
//for use in memory leak analysis
void* alloc_named(unsigned int size, const char* name);

//alloc can be used in place of malloc
void* alloc(unsigned int size);

//dealloc can be used in place of free
void dealloc(void* ptr);

//This function returns the amound currently allocated using alloc()
unsigned int allocated();

//This function returns the maximum allocated at once during entire operation
unsigned int peak_allocated();

void print_memory_usage_stats();

void named_allocation_dump();

//returns true if ptr is in list of allocated pointers, false otherwise
uint8_t is_valid_pointer(void* ptr);

#else  //if not using memory trakcing, replace alloc calls with malloc etc

#include <stdlib.h>
#define alloc_named(X, Y) malloc(X)
#define alloc(X) malloc(X)
#define dealloc(X) free(X)
#define allocated() 0
#define peak_allocated() 0
#define print_memory_usage_stats() printf("Memory usage not tracked, use #define USE_MEMORY_TRACKING in memory_tracker.h to change.\n");
#define named_allocation_dump() ;
#define is_valid_pointer(X) 1

#endif  //USE_MEMORY_TRACKING

#endif  //MEMORY_TRACKER_H
