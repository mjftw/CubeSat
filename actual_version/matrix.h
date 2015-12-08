#ifndef MATRIX_H
#define MATRIX_H

//uses raw_data type to represent a matrix- all matricies are square
#include "datatypes.h"

//uses galois field arithmetic on all matrices

uint8_t determinant(raw_data mat);

//returns the inverse of a matrix
raw_data inverse(raw_data mat, uint8_t det);

//multiplies a nxn matrix by a length n vector and returns a length n vector
raw_data mat_vec_multiply(raw_data mat, raw_data vec);

//returns the pointer to the matrix element at row, column
uint8_t* ptr_at(raw_data mat, uint8_t row, uint8_t col);

//returns the value of the matrix element at row, column
uint8_t at(raw_data mat, uint8_t row, uint8_t col);

void print_matrix(raw_data mat);

#endif  //MATRIX_H
