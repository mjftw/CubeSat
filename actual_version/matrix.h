#ifndef MATRIX_H
#define MATRIX_H

//uses raw_data type to represent a matrix- all matricies are square
#include "datatypes.h"

//uses galois field arithmetic on all matrices

uint8_t determinant(raw_data mat);

//returns the inverse of a matrix
raw_data inverse(raw_data mat, uint8_t det);

#endif  //MATRIX_H
