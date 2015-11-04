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
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)alloc_named(rd2.length, "interleave rd2.data");
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  unsigned int position = 0;
  for(unsigned int i = 0; i < 8; i++)  //for each bit
  {
    for(unsigned int j = 0; j < rd.length; j++)  //in each byte
    {
      insert_bits_at_position(rd2.data, get_bit(rd.data[j], 7 - i), 1, &position);
    }
  }
  memcpy(rd.data, rd2.data, rd.length);
  dealloc(rd2.data);
}

void deinterleave(raw_data rd)
{
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)alloc_named(rd2.length, "deinterleave rd2.data");
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  unsigned int position = 0;
  unsigned int position2 = 0;
  for(unsigned int i = 0; i < rd.length * 8; i++)  //for each bit
  {
    int a = i / 8;
    int b = i % 8;

    position = a + b * rd.length;
    uint8_t bit = get_bits_from_position(rd.data, 1, &position);
    //bit position is 7 - b because bit 0 is LSB (leftmost) and we want to read
    //right to left
    insert_bits_at_position(rd2.data, bit, 1, &position2);
  }
  memcpy(rd.data, rd2.data, rd.length);
  dealloc(rd2.data);
}
