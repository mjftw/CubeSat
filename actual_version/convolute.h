#ifndef CONVOLUTE_H
#define CONVOLUTE_H

#include "datatypes.h"

//returns convoluted data.
raw_data convolute(raw_data rd);

//returns deconvoluted data, increases bit_error_count
//with expected count of bit errors
raw_data deconvolute(raw_data rd, int* bit_error_count);




#endif  //CONVOLUTE_H
