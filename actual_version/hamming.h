#ifndef HAMMING_H
#define HAMMING_H

//This header file contains the API for encoding and decoding blocks of bytes
//using hamming47 coding

#include "datatypes.h"  //for raw_data struct

//encodes a block using hamming47 encoding
raw_data encode_block(raw_data rd);
//decodes a block usiong hamming47 encoding
raw_data decode_block(raw_data rd, int* bit_errors);

#endif  //HAMMING_H
