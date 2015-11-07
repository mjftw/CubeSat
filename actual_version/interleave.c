#include "interleave.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bitstream.h"
#include "memory_tracker.h"

uint8_t get_bit(uint8_t byte, int position)
{
  return (byte & 1 << position) ? 1 : 0;
}

void interleave(raw_data rd)
{
  uint8_t interleave_16 = 0;
  if(rd.length / 2 == 0)
    interleave_16 = 1;  //else interleave on 8 bits
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)alloc_named(rd2.length, "interleave rd2.data");
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  unsigned int position = 0;
  for(unsigned int i = 0; i < (interleave_16 ? 16 : 8); i++)  //for each bit
  {
    for(unsigned int j = 0; j < rd.length; j += (interleave_16 ? 2 : 1))  //in each byte two bytes
    {
      insert_bits_at_position(rd2.data, get_bit(rd.data[j + i / 8], (15 - i) % 8), 1, &position);
    }
  }
  //may be possible to optimise out the memcpy by copying pointer
  memcpy(rd.data, rd2.data, rd.length);
  dealloc(rd2.data);
}

void deinterleave(raw_data rd)
{
  uint8_t interleave_16 = 0;
  if(rd.length / 2 == 0)
    interleave_16 = 1;  //else interleave on 8 bits
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)alloc_named(rd2.length, "deinterleave rd2.data");
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  unsigned int position = 0;
  unsigned int position2 = 0;
  for(unsigned int i = 0; i < rd.length * 8; i++)  //for each bit
  {
    int a = i / (interleave_16 ? 16 : 8);
    int b = i % (interleave_16 ? 16 : 8);

    if(interleave_16)
      position = a + b * rd.length / 2;
    else
      position = a + b * rd.length;
    //printf("%i ", position);
    uint8_t bit = get_bits_from_position(rd.data, 1, &position);
    //bit position is 7 - b because bit 0 is LSB (leftmost) and we want to read
    //right to left
    insert_bits_at_position(rd2.data, bit, 1, &position2);
  }
  memcpy(rd.data, rd2.data, rd.length);
  dealloc(rd2.data);
}
