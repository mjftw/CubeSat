#include "memory_tracker.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifdef USE_MEMORY_TRACKING

unsigned int total_allocated_space = 0;
unsigned int peak_allocated_space = 0;
unsigned int alloc_calls = 0;
unsigned int dealloc_calls = 0;

typedef struct
{
  void* ptr;
  unsigned int size;
  const char* name;
} mem_alloc;

unsigned int total_allocated = 0;  //number of allocations made
mem_alloc* allocations = NULL;

void* alloc_named(unsigned int size, const char* name)
{
  void* ptr = alloc(size);
  allocations[total_allocated-1].name = name;
  return ptr;
}

//alloc can be used in place of malloc
void* alloc(unsigned int size)
{
  alloc_calls++;
  total_allocated_space += size;
  if(total_allocated_space > peak_allocated_space)
    peak_allocated_space = total_allocated_space;

  if(allocations == NULL)
  {
    allocations = (mem_alloc*)malloc(10 * sizeof(mem_alloc));
  }
  else if(total_allocated % 10 == 0)
  {
    //already allocated allocations, but need more
    mem_alloc* allocations2 = (mem_alloc*)malloc((total_allocated + 10) * sizeof(mem_alloc));
    memcpy(allocations2, allocations, total_allocated * sizeof(mem_alloc));
    free(allocations);
    allocations = allocations2;
  }

  void* ptr = malloc(size);

  allocations[total_allocated].ptr = ptr;
  allocations[total_allocated].size = size;
  allocations[total_allocated].name = NULL;  //default, overridden by alloc_named

  total_allocated++;
  return ptr;
}

void dealloc_named(void* ptr, const char* name)
{
  //reverse search-- assume last to be allocated will be first to be freed
  dealloc_calls++;
  int i;
  for(i = total_allocated-1; i >= 0; i--)
  {
    if(allocations[i].ptr == ptr)
    {
      total_allocated_space -= allocations[i].size;
      free(ptr);
      total_allocated--;
      //move last in list into space- makes order less ideal but easier than moving all
      //efficiency is not a main concern here, these commands can be bypassed if need be.
      memcpy(&(allocations[i]), &(allocations[total_allocated]), sizeof(mem_alloc));
      return;
    }
  }
  printf("ERROR: dealloc called with non- allocated pointer as argument\n");
  printf(name);
  printf(": %p\n", ptr);
  assert(0);  //should never reach here
}

//dealloc can be used in place of free
void dealloc(void* ptr)
{
  //reverse search-- assume last to be allocated will be first to be freed
  dealloc_calls++;
  int i;
  for(i = total_allocated-1; i >= 0; i--)
  {
    if(allocations[i].ptr == ptr)
    {
      total_allocated_space -= allocations[i].size;
      free(ptr);
      total_allocated--;
      //move last in list into space- makes order less ideal but easier than moving all
      //efficiency is not a main concern here, these commands can be bypassed if need be.
      memcpy(&(allocations[i]), &(allocations[total_allocated]), sizeof(mem_alloc));
      return;
    }
  }
  printf("ERROR: dealloc called with non- allocated pointer as argument\n");
  printf("%p\n", ptr);
  assert(0);  //should never reach here
}

unsigned int allocated()
{
  return total_allocated_space;
}

unsigned int peak_allocated()
{
  return peak_allocated_space;
}

void print_memory_usage_stats()
{
  printf("currently allocated space = %i B\n", total_allocated_space);
  printf("peak allocated space      = %i B\n", peak_allocated_space);
  printf("calls to alloc            = %i\n", alloc_calls);
  printf("calls to dealloc          = %i\n", dealloc_calls);
  if(total_allocated > 0)
  {
    printf("printing all allocated details for memory leak analysis\n");
    unsigned int i;
    for(i = 0; i < total_allocated; i++)
    {
      printf("%i:\t", i);
      if(allocations[i].name)
        printf("Name: %s\n\t", allocations[i].name);
      printf("Size: %i\n", allocations[i].size);
    }
  }
}

void named_allocation_dump()
{
  unsigned int i;
  for(i = 0; i < total_allocated; i++)
  {
    printf("%i:\t", i);
    if(allocations[i].name)
      printf("Name: %s\n\t", allocations[i].name);
    printf("Size: %i\n", allocations[i].size);
  }
}

uint8_t is_valid_pointer(void* ptr)
{
  unsigned int i;
  for(i = 0; i < total_allocated; i++)
  {
    if(allocations[i].ptr == ptr)
      return 1;
  }
  return 0;
}

#else

#include "uart_funcs.h"

void* alloc_named(unsigned int size, const char* name)
{
  void* ptr = malloc(size);
  if(ptr == NULL)
  {
    UART_TX_string("failed to allocate memory:\n\r\t");
    UART_TX_string(name);
    char tmp[25];
    sprintf(tmp, "\tsize: %i bytes\n\r", size);
    UART_TX_string(tmp);
  }
  return ptr;
}

//alloc can be used in place of malloc
void* alloc(unsigned int size)
{
  void* ptr = malloc(size);
  if(ptr == NULL)
  {
    UART_TX_string("failed to allocate memory:\n\r\t");
    char tmp[25];
    sprintf(tmp, "size: %i bytes\n\r", size);
    UART_TX_string(tmp);
  }
  return ptr;
}

#endif  //USE_MEMORY_TRACKING
