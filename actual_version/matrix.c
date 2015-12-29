#include "matrix.h"
#include <math.h>
#include <stdio.h>
#include "galois_field.h"
#include "memory_tracker.h"

//internal data representation is rows then columns
/*
  data[0]  data[1]  data[2]
  data[3]  data[4]  data[5]
  data[6]  data[7]  data[8]
*/

void print_matrix(raw_data mat)
{
  uint8_t size = sqrt(mat.length);
  for(unsigned int i = 0; i < size; i++)
  {
    for(unsigned int j = 0; j < size; j++)
    {
      int chars = printf("%x ", mat.data[i * size + j]);
      if(chars == 2)
        printf(" ");
    }
    printf("\n");
  }
}

uint8_t* ptr_at(raw_data mat, uint8_t row, uint8_t col)
{
  uint8_t size = sqrt(mat.length);
  return &(mat.data[row * size + col]);
}

uint8_t at(raw_data mat, uint8_t row, uint8_t col)
{
  uint8_t size = sqrt(mat.length);
  return mat.data[row * size + col];
}

//returns the sub-matrix produced by remove row and column. Will not edit mat
raw_data sub_matrix(raw_data mat, uint8_t row, uint8_t col)
{
  raw_data ret;
  ret.length = pow(sqrt(mat.length) - 1, 2);
  ret.data = (uint8_t*)alloc_named(ret.length, "sub_matrix ret");
  uint8_t size = sqrt(mat.length);

  uint8_t put_p = 0;
  for(unsigned int i = 0; i < mat.length; i++)
  {
    if(i % size == col);  //remove
    else if(i / size == row);  //remove
    else
      ret.data[put_p++] = mat.data[i];
  }
  return ret;
}

uint8_t determinant(raw_data mat)
{
  if(mat.length == 0)
    return 1;
  else if(mat.length == 1)
    return mat.data[0];
  else if(mat.length == 4)
  {
    return galois_multiply(mat.data[0], mat.data[3])
      ^ galois_multiply(mat.data[1], mat.data[2]);
  }
  else if(mat.length == 9)
  {
    uint8_t a = galois_multiply(mat.data[0],
        galois_multiply(mat.data[4], mat.data[8])
      ^ galois_multiply(mat.data[5], mat.data[7]));
    uint8_t b = galois_multiply(mat.data[1],
        galois_multiply(mat.data[3], mat.data[8])
      ^ galois_multiply(mat.data[5], mat.data[6]));
    uint8_t c = galois_multiply(mat.data[2],
        galois_multiply(mat.data[3], mat.data[7])
      ^ galois_multiply(mat.data[4], mat.data[6]));
    return a ^ b ^ c;
  }

  uint8_t size = sqrt(mat.length);
  uint8_t accumulator = 0;
  for(unsigned int i = 0; i < size; i++)
  {
    raw_data sub_mat = sub_matrix(mat, 0, i);
    accumulator ^= galois_multiply(mat.data[i], determinant(sub_mat));
    dealloc(sub_mat.data);
  }

  return accumulator;
}

//returns the inverse of a matrix
raw_data inverse(raw_data mat, uint8_t det)
{
  raw_data cofactor;
  cofactor.length = mat.length;
  cofactor.data = (uint8_t*)alloc_named(cofactor.length, "inverse cofactor");
  uint8_t size = sqrt(mat.length);

  //calculate matrix of cofactors
  for(uint8_t i = 0; i < size; i++)
  {
    for(uint8_t j = 0; j < size; j++)
    {
      raw_data sub_mat = sub_matrix(mat, i, j);
      cofactor.data[i * size + j] = determinant(sub_mat);
      dealloc(sub_mat.data);
    }
  }

  //reflect on diagonal
  for(uint8_t i = 0; i < size; i++)
  {
    for(uint8_t j = i+1; j < size; j++)
    {
      uint8_t tmp = at(cofactor, j, i);
      *ptr_at(cofactor, j, i) = at(cofactor, i, j);
      *ptr_at(cofactor, i, j) = tmp;
    }
  }
  raw_data ret;
  ret.length = mat.length;
  ret.data = (uint8_t*)alloc_named(ret.length, "inverse ret");

  for(unsigned int i = 0; i < cofactor.length; i++)
    ret.data[i] = galois_divide(cofactor.data[i], det);

  dealloc(cofactor.data);

  return ret;
}

//multiplies a nxn matrix by a length n vector and returns a length n vector
raw_data mat_vec_multiply(raw_data mat, raw_data vec)
{
  raw_data ret;
  ret.length = vec.length;
  ret.data = (uint8_t*)alloc_named(ret.length, "mat_vec_multiply ret");

  for(unsigned int i = 0; i < ret.length; i++)
  {
    uint8_t accumulator = 0;
    for(unsigned int j = 0; j < ret.length; j++)
    {
      accumulator ^= galois_multiply(at(mat, i, j), vec.data[j]);
    }
    ret.data[i] = accumulator;
  }
  return ret;
}
