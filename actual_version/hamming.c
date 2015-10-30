#include "hamming.h"
#include <stdlib.h>
#include <stdio.h>

//encodes 4 bits to 7 using Hamming(4,7) ECC
uint8_t hamming47_lookup(uint8_t input)
{
	static const char lookup[] = {
		0x0, 0x70, 0x4c, 0x3c, 0x2a, 0x5a, 0x66, 0x16,
		0x69, 0x19, 0x25, 0x55, 0x43, 0x33, 0xf, 0x7f};

	return lookup[input];
}

//returns the corrected version (assuming 1 or 0 bit errors) from 7 bit section
uint8_t check_hamming47_lookup(uint8_t input)
{
	static const uint8_t lookup[] = {
		0x0, 0x0, 0x0, 0xc, 0x0, 0xa, 0x7, 0xe,
		0x0, 0x9, 0x4, 0xe, 0x2, 0xe, 0xe, 0xe,
		0x0, 0x9, 0x7, 0xd, 0x7, 0xb, 0x7, 0x7,
		0x9, 0x9, 0x5, 0x9, 0x3, 0x9, 0x7, 0xe,
		0x0, 0xa, 0x4, 0xd, 0xa, 0xa, 0x6, 0xa,
		0x4, 0x8, 0x4, 0x4, 0x3, 0xa, 0x4, 0xe,
		0x1, 0xd, 0xd, 0xd, 0x3, 0xa, 0x7, 0xd,
		0x3, 0x9, 0x4, 0xd, 0x3, 0x3, 0x3, 0xf,
		0x0, 0xc, 0xc, 0xc, 0x2, 0xb, 0x6, 0xc,
		0x2, 0x8, 0x5, 0xc, 0x2, 0x2, 0x2, 0xe,
		0x1, 0xb, 0x5, 0xc, 0xb, 0xb, 0x7, 0xb,
		0x5, 0x9, 0x5, 0x5, 0x2, 0xb, 0x5, 0xf,
		0x1, 0x8, 0x6, 0xc, 0x6, 0xa, 0x6, 0x6,
		0x8, 0x8, 0x4, 0x8, 0x2, 0x8, 0x6, 0xf,
		0x1, 0x1, 0x1, 0xd, 0x1, 0xb, 0x6, 0xf,
		0x1, 0x8, 0x5, 0xf, 0x3, 0xf, 0xf, 0xf};

	return lookup[input];
}

//data is data stream to insert into, bits is literal bits, num_bits is number of bits (<=8)
//position is position in data stream in bits
void insert_bits_at_position(uint8_t* data, uint8_t bits, int num_bits, int* position)
{
  data += *position / 8;
  int bit_position = *position % 8;
  uint8_t bits_copy = bits;  //bits may be needed unaltered later when spanning bytes
  bits_copy <<= 8 - num_bits;
  bits_copy >>= bit_position;
  *data |= bits_copy;  //leave rest of byte unaltered
  if(bit_position > 8 - num_bits)
  {
    //need to put lower part of bits in next byte
    int bits_left = bit_position - (8 - num_bits);
    bits <<= 8 - bits_left;
    //no need to shift left, as bits will start at beginning of byte (MSB)
    *(data+1) |= bits;  //leave rest of byte unaltered
  }
  *position += num_bits;
}

uint8_t get_bits_from_position(const uint8_t* data, int num_bits, int* position)
{
  int bits_left_in_byte = 8 - (*position % 8);
  if(bits_left_in_byte > num_bits)
    bits_left_in_byte = num_bits;
  int bits_in_next_byte = num_bits - bits_left_in_byte;

  uint8_t ret;
  if(*position % 8 == 0)
    ret = data[*position / 8] >> 1;
  else
    ret = (data[*position / 8] & ((1 << bits_left_in_byte) - 1)) << bits_in_next_byte;

  if(bits_in_next_byte > 0)
  {
    //extract more from next byte
    ret |= data[*position / 8 + 1] >> (8 - bits_in_next_byte);
  }
  *position += num_bits;

  return ret;
}

raw_data encode_block(raw_data rd)
{
  raw_data ret;
  float bytes_needed = ((float)rd.length * 7.0 / 4.0);
  if(bytes_needed != (float)((int)bytes_needed));  //test if it fits exactly
    bytes_needed += 1;
  ret.length = (int)bytes_needed;
  ret.data = (uint8_t*)malloc(ret.length);
  for(unsigned int i = 0; i < ret.length; i++)
    ret.data[i] = 0;

  int position = 0;  //position will be automatically changed by insert_bits_at_position
  for(unsigned int i = 0; i < rd.length; i++)
  {
    uint8_t first = hamming47_lookup(rd.data[i] >> 4);
    uint8_t second = hamming47_lookup(rd.data[i] & 0xf);
    insert_bits_at_position(ret.data, first, 7, &position);
    insert_bits_at_position(ret.data, second, 7, &position);
  }
  return ret;
}

raw_data decode_block(raw_data rd, int* bit_errors)
{
  //counting the bit_errors doesn't work currently, need another function to do it.
  //Not too difficult using the lookup table writing funciton in ECC beginning

  raw_data ret;
  ret.length = (int)((float)rd.length / 7.0 * 4.0);
  ret.data = (uint8_t*)malloc(ret.length);
  for(unsigned int i = 0; i < ret.length; i++)
    ret.data[i] = 0;

  int position = 0, decoded_position = 0;
  while(position / 8 < rd.length)
  {
    uint8_t encoded = get_bits_from_position(rd.data, 7, &position);
    uint8_t decoded = check_hamming47_lookup(encoded);
    if(bit_errors != NULL && hamming47_lookup(decoded) != encoded)
    {
      (*bit_errors)++;
    }
    insert_bits_at_position(ret.data, decoded, 4, &decoded_position);
  }
  return ret;
}
