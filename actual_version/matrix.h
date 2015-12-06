#ifndef MATRIX_H
#define MATRIX_H

//uses raw_data type to represent a matrix
#include "datatypes.h"

//uses galois field arithmetic on all matrices

uint8_t determinant(raw_data mat);

//inverts a matrix in place
void inverse(raw_data mat);

#endif  //MATRIX_H
