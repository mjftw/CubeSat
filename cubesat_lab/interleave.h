#ifndef INTERLEAVE_H
#define INTERLEAVE_H

#include "datatypes.h"

//interleave and deinterleave will modify raw_data rd in place, as rd contains
//a pointer

void interleave(raw_data rd);

void deinterleave(raw_data rd);

#endif  //INTERLEAVE_H
