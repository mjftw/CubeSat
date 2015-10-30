
//allows bits to be inserted or got from a position in a block of memory.
//functions that take int* position will modify position by the number
//of bits stored or retrieved

#ifndef BITSTREAM_H
#define BITSTREAM_H

void insert_bits_at_position(uint8_t* data, uint8_t bits, int num_bits, int* position);

uint8_t get_bits_from_position(const uint8_t* data, int num_bits, int* position);

#endif  //BITSTREAM_H
