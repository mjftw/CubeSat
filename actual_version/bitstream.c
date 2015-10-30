

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
