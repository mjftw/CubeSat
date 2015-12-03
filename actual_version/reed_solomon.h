#ifndef REED_SOLOMON_H
#define REED_SOLOMON_H

#include "datatypes.h"

/*  This implementation of Reed-Solomon codes uses 8 bit symbols,
 *  and the number of parity symbols is configurable
 */

raw_data rs_encode(raw_data rd, int t);

raw_data rs_decode(raw_data rd, int* bit_error_count);


#endif  //REED_SOLOMON_H
