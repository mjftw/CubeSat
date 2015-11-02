#include "convolute.h"
#include <stdlib.h>
#include "bitstream.h"
#include "memory_tracker.h"

//currently uses (2,1,4) convolution

//inserts a bit into the encoder state
void insert_bit(uint8_t* encoder_state, uint8_t bit)
{
  *encoder_state >>= 1;
  *encoder_state |= bit << 3;
}

uint8_t encoder_lookup(uint8_t encoder_state)
{
  //v0 generated as u0 xor u1 xor u2 xor u3
  //v1 generated as u0 xor u1 xor u3
  uint8_t lookup[] = {
      0x00, 0x03, 0x02, 0x01, 0x03, 0x00, 0x01, 0x02,
      0x03, 0x00, 0x01, 0x02, 0x00, 0x03, 0x02, 0x01};

  return lookup[encoder_state];
}

raw_data convolute(raw_data rd)
{
  uint8_t encoder_state = 0;  //4 bits due to (2,1,4) convolution
  raw_data ret;
  ret.length = rd.length * 2;  //2:1 ratio (code rate = 1/2)
  ret.data = (uint8_t*)alloc_named(ret.length, "convolute ret.data");
  for(unsigned int i = 0; i < ret.length; i++)
    ret.data[i] = 0;
  int position = 0;
  int position2 = 0;
  for(unsigned int i = 0; i < rd.length * 8; i++)
  {
    uint8_t bit = get_bits_from_position(rd.data, 1, &position);
    insert_bit(&encoder_state, bit);
    insert_bits_at_position(ret.data, encoder_lookup(encoder_state), 2, &position2);
  }
  return ret;
}

raw_data deconvolute(raw_data rd, int* bit_error_count)
{

}
