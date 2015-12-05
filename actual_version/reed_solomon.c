#include "reed_solomon.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "datatypes.h"
#include "memory_tracker.h"

//for GF(256), a suitable field generator polynomial is x^8 + x^4 + x^3 + x^2 + 1 = 0
//this is irreductible (has no factors).

//returns the element in GF(256) at index, generated by make_lookup_table
uint8_t GF256(uint8_t index)
{
	static const uint8_t lookup[] = {
		0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40,
		0x80, 0x1d, 0x3a, 0x74, 0xe8, 0xcd, 0x87, 0x13,
		0x26, 0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea,
		0xc9, 0x8f, 0x3, 0x6, 0xc, 0x18, 0x30, 0x60,
		0xc0, 0x9d, 0x27, 0x4e, 0x9c, 0x25, 0x4a, 0x94,
		0x35, 0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f,
		0x23, 0x46, 0x8c, 0x5, 0xa, 0x14, 0x28, 0x50,
		0xa0, 0x5d, 0xba, 0x69, 0xd2, 0xb9, 0x6f, 0xde,
		0xa1, 0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e,
		0xbc, 0x65, 0xca, 0x89, 0xf, 0x1e, 0x3c, 0x78,
		0xf0, 0xfd, 0xe7, 0xd3, 0xbb, 0x6b, 0xd6, 0xb1,
		0x7f, 0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71,
		0xe2, 0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44,
		0x88, 0xd, 0x1a, 0x34, 0x68, 0xd0, 0xbd, 0x67,
		0xce, 0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7,
		0x93, 0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66,
		0xcc, 0x85, 0x17, 0x2e, 0x5c, 0xb8, 0x6d, 0xda,
		0xa9, 0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a,
		0x54, 0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55,
		0xaa, 0x49, 0x92, 0x39, 0x72, 0xe4, 0xd5, 0xb7,
		0x73, 0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f,
		0x7e, 0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1,
		0xff, 0xe3, 0xdb, 0xab, 0x4b, 0x96, 0x31, 0x62,
		0xc4, 0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae,
		0x41, 0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x7,
		0xe, 0x1c, 0x38, 0x70, 0xe0, 0xdd, 0xa7, 0x53,
		0xa6, 0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9,
		0xef, 0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a,
		0x9, 0x12, 0x24, 0x48, 0x90, 0x3d, 0x7a, 0xf4,
		0xf5, 0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0xb,
		0x16, 0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf,
		0x83, 0x1b, 0x36, 0x6c, 0xd8, 0xad, 0x47, 0x8e};

	return lookup[index];
}

//returns the index of GF element, generated by make_lookup_table
uint8_t GF256inv(uint8_t index)
{
	static const uint8_t lookup[] = {
		0x0, 0x1, 0x2, 0x1a, 0x3, 0x33, 0x1b, 0xc7,
		0x4, 0xe0, 0x34, 0xef, 0x1c, 0x69, 0xc8, 0x4c,
		0x5, 0x65, 0xe1, 0xf, 0x35, 0x8e, 0xf0, 0x82,
		0x1d, 0xc2, 0x6a, 0xf9, 0xc9, 0x9, 0x4d, 0x72,
		0x6, 0x8b, 0x66, 0x30, 0xe2, 0x25, 0x10, 0x22,
		0x36, 0x94, 0x8f, 0xdb, 0xf1, 0x13, 0x83, 0x46,
		0x1e, 0xb6, 0xc3, 0x7e, 0x6b, 0x28, 0xfa, 0xba,
		0xca, 0x9b, 0xa, 0x79, 0x4e, 0xe5, 0x73, 0xa7,
		0x7, 0xc0, 0x8c, 0x63, 0x67, 0xde, 0x31, 0xfe,
		0xe3, 0x99, 0x26, 0xb4, 0x11, 0x92, 0x23, 0x89,
		0x37, 0xd1, 0x95, 0xcf, 0x90, 0x97, 0xdc, 0xbe,
		0xf2, 0xd3, 0x14, 0x5d, 0x84, 0x39, 0x47, 0x41,
		0x1f, 0x43, 0xb7, 0xa4, 0xc4, 0x49, 0x7f, 0x6f,
		0x6c, 0x3b, 0x29, 0x55, 0xfb, 0x86, 0xbb, 0x3e,
		0xcb, 0x5f, 0x9c, 0xa0, 0xb, 0x16, 0x7a, 0x2c,
		0x4f, 0xd5, 0xe6, 0xad, 0x74, 0xf4, 0xa8, 0x58,
		0x8, 0x71, 0xc1, 0xf8, 0x8d, 0x81, 0x64, 0xe,
		0x68, 0x4b, 0xdf, 0xee, 0x32, 0xc6, 0xff, 0x19,
		0xe4, 0xa6, 0x9a, 0x78, 0x27, 0xb9, 0xb5, 0x7d,
		0x12, 0x45, 0x93, 0xda, 0x24, 0x21, 0x8a, 0x2f,
		0x38, 0x40, 0xd2, 0x5c, 0x96, 0xbd, 0xd0, 0xce,
		0x91, 0x88, 0x98, 0xb3, 0xdd, 0xfd, 0xbf, 0x62,
		0xf3, 0x57, 0xd4, 0xac, 0x15, 0x2b, 0x5e, 0x9f,
		0x85, 0x3d, 0x3a, 0x54, 0x48, 0x6e, 0x42, 0xa3,
		0x20, 0x2e, 0x44, 0xd9, 0xb8, 0x7c, 0xa5, 0x77,
		0xc5, 0x18, 0x4a, 0xed, 0x80, 0xd, 0x70, 0xf7,
		0x6d, 0xa2, 0x3c, 0x53, 0x2a, 0x9e, 0x56, 0xab,
		0xfc, 0x61, 0x87, 0xb2, 0xbc, 0xcd, 0x3f, 0x5b,
		0xcc, 0x5a, 0x60, 0xb1, 0x9d, 0xaa, 0xa1, 0x52,
		0xc, 0xf6, 0x17, 0xec, 0x7b, 0x76, 0x2d, 0xd8,
		0x50, 0xaf, 0xd6, 0xea, 0xe7, 0xe8, 0xae, 0xe9,
		0x75, 0xd7, 0xf5, 0xeb, 0xa9, 0x51, 0x59, 0xb0};

	return lookup[index];
}

//returns a * b
uint8_t galois_multiply(uint8_t a, uint8_t b)
{
	if(a == 0 || b == 0)
		return 0;
	uint16_t ret = GF256((GF256inv(a) + GF256inv(b) - 1) % 255);

	if(ret == 0)
		return 142;  //the logarithmic method doesn't work and produces 0's- 142 is always correct in this case
	return ret;
}

//return a / b
uint8_t galois_divide(uint8_t a, uint8_t b)
{
	//potential problem with 0
	if(a == 0 || b == 0)
		return 0;
	int16_t c = GF256inv(a) - GF256inv(b);
	if(c < 0)
		c += 255;
	return GF256(c+1);
}

raw_data produce_generator_polynomial(int t)
{
	//start with factors like (x+GF256(1)(x+GF256(2)) etc
	//expand factors into each other
	raw_data factorised;
	factorised.length = 2 * t;
	factorised.data = (uint8_t*)alloc_named(factorised.length, "produce_generator_polynomial factorised");
	for(unsigned int i = 0; i < factorised.length; i++)
		factorised.data[i] = GF256(i+1);

	raw_data ret, aux;
	ret.length = 2 * t + 1;
	ret.data = (uint8_t*)alloc_named(ret.length, "produce_generator_polynomial ret");
	aux.length = 2 * t + 1;
	aux.data = (uint8_t*)alloc_named(ret.length, "produce_generator_polynomial aux");

	//ret and aux represent polynomials. The index represents the power and the
	//value represents the coefficient. aux is auxiliary for shift and add
	ret.data[0] = 1;
	for(unsigned int i = 1; i < ret.length; i++)
	{
		ret.data[i] = 0;
	}

	for(unsigned int i = 0; i < factorised.length; i++)
	{
		//multiply - put result in aux
		for(unsigned int j = 0; j < ret.length; j++)
			aux.data[j] = galois_multiply(ret.data[j], factorised.data[i]);

		//shift in place
		for(unsigned int j = ret.length - 1; j > 0; j--)
			ret.data[j] = ret.data[j-1];
		ret.data[0] = 0;

		//add
		for(unsigned int j = 0; j < ret.length; j++)
			ret.data[j] ^= aux.data[j];
	}

	dealloc(aux.data);
	dealloc(factorised.data);

	//reverse in place
	for(unsigned int i = 0; i < ret.length / 2; i++)
	{
		uint8_t tmp = ret.data[i];
		ret.data[i] = ret.data[ret.length - 1 - i];
		ret.data[ret.length - 1 - i] = tmp;
	}

  return ret;
}

//the encode function simply calculates the 2t parity checking symbols and returns T(x)
raw_data rs_encode(raw_data rd, int t)
{
  raw_data ret;

	ret.length = rd.length + 2 * t;
	ret.data = (uint8_t*)alloc_named(ret.length, "rs_encode ret");

	memcpy(ret.data, rd.data, rd.length);  //copy message
	for(unsigned int i = rd.length; i < ret.length; i++)
		ret.data[i] = 0;  //initialise parity symbols to 0

	raw_data g_x = produce_generator_polynomial(t);

	//do division here-
	for(unsigned int i = 0; i < ret.length - 2 * t; i++)
	{
		if(ret.data[i] == 0)  //multiply yields 0, so subtract doesn't change it
			continue;
		uint8_t multiplier = ret.data[i];
		for(unsigned int j = 0; j < g_x.length; j++)
			ret.data[i+j] ^= galois_multiply(g_x.data[j], multiplier);
	}

	//copy message back- as message symbols are unchanged in T(x)
	memcpy(ret.data, rd.data, rd.length);

	dealloc(g_x.data);
  return ret;
}

raw_data rs_decode(raw_data rd, int* bit_error_count)
{
  raw_data ret;

  return ret;
}
