#include "interleave.h"
#include <stdlib.h>
#include <string.h>
#include "bitstream.h"

uint8_t get_bit(uint8_t byte, int position)
{
  return (byte & 1 << position) ? 1 : 0;
}

void interleave(raw_data rd)
{
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)malloc(rd2.length);
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  int position = 0;
  for(unsigned int i = 0; i < 8; i++)  //for each bit
  {
    for(unsigned int j = 0; j < rd.length; j++)  //in each byte
    {
      insert_bits_at_position(rd2.data, get_bit(rd.data[j], i), 1, &position);
    }
  }
  memcpy(rd.data, rd2.data, rd.length);
  free(rd2.data);
}

void deinterleave(raw_data rd)
{
  raw_data rd2;
  rd2.length = rd.length;
  rd2.data = (uint8_t*)malloc(rd2.length);
  for(unsigned int i = 0; i < rd.length; i++)
    rd2.data[i] = 0x00;

  int position = 0;
  for(unsigned int i = 0; i < rd.length * 8; i++)  //for each bit
  {
    int a = i / 8;
    int b = i % 8;
    position = 7 - a + b * rd.length;
    //bit position is 7 - b because bit 0 is LSB (leftmost) and we want to read
    //right to left
    insert_bits_at_position(rd2.data, get_bit(rd.data[a], 7 - b), 1, &position);
  }
  memcpy(rd.data, rd2.data, rd.length);
  free(rd2.data);
}
