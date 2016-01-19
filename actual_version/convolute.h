#ifndef CONVOLUTE_H
#define CONVOLUTE_H

#include "datatypes.h"

//returns convoluted data.
raw_data convolute(raw_data rd);

//returns deconvoluted data, increases bit_error_count
//with expected count of bit errors, bit_error_count is a lower bound:
//there can be more errors but these are corrected by random chance
//bit_error_count is incorrect when a false decoding occurs
raw_data deconvolute(raw_data rd, int* bit_error_count);

//constrained versions do convolution on blocks of length constraint_length
//This makes decoding more efficient time-wise, at the slight expense of
//encoding time and space efficiency
//constraint_length is in bytes
raw_data convolute_constrained(raw_data rd, unsigned int constraint_length);
raw_data deconvolute_constrained(raw_data rd, int* bit_error_count, unsigned int constraint_length);

#endif  //CONVOLUTE_H
