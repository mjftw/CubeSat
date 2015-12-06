#ifndef GALOIS_FIELD_H
#define GALOIS_FIELD_H

#include "datatypes.h"

//returns the galois field elements at index (or inverse)
uint8_t GF256(uint8_t index);
uint8_t GF256inv(uint8_t index);

//multiplication and division in galois fields
uint8_t galois_multiply(uint8_t a, uint8_t b);
//returns a / b
uint8_t galois_divide(uint8_t a, uint8_t b);

//for addition/subtraction of a and b, use (a ^ b)


#endif  //GALOIS_FIELD_H
