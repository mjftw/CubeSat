#ifndef CONVOLUTE_H
#define CONVOLUTE_H

#include "datatypes.h"

//returns convoluted data.
raw_data convolute(raw_data rd);

//returns deconvoluted data, increases bit_error_count
//with expected count of bit errors, bit_error_count is a lower bound:
//there can be more errors but these are corrected by random chance
raw_data deconvolute(raw_data rd, int* bit_error_count);

unsigned int get_peak_num_paths_considered();
void reset_peak_num_paths_considered();



#endif  //CONVOLUTE_H
