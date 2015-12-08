#include "reed_solomon.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "datatypes.h"
#include "memory_tracker.h"
#include "galois_field.h"
#include "matrix.h"

raw_data get_factorised_generator(int t)
{
	raw_data factorised;
	factorised.length = 2 * t;
	factorised.data = (uint8_t*)alloc_named(factorised.length, "produce_generator_polynomial factorised");
	for(unsigned int i = 0; i < factorised.length; i++)
		factorised.data[i] = GF256(i+1);
	return factorised;
}

raw_data produce_generator_polynomial(int t)
{
	//start with factors like (x+GF256(1)(x+GF256(2)) etc
	//expand factors into each other
	raw_data factorised = get_factorised_generator(t);

	raw_data ret, aux;
	ret.length = 2 * t + 1;
	ret.data = (uint8_t*)alloc_named(ret.length, "produce_generator_polynomial ret");
	aux.length = 2 * t + 1;
	aux.data = (uint8_t*)alloc_named(aux.length, "produce_generator_polynomial aux");

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

raw_data rs_decode(raw_data rd, int t, int* bit_error_count)
{
  raw_data ret;

	//resuse rs_encode to recalculate parity bits for received code
	rd.length -= 2 * t;
	raw_data re_encoded = rs_encode(rd, t);
	raw_data parity_bits;
	parity_bits.length = 2 * t;
	parity_bits.data = (uint8_t*)alloc_named(parity_bits.length, "rs_decode parity_bits");
	memcpy(parity_bits.data, re_encoded.data + re_encoded.length - 2 * t, 2 * t);
	dealloc(re_encoded.data);
	rd.length += 2 * t;

	//calculate syndromes: this is done by dividing the received polynomial by
	//all the factors of the generator polynomial. The simpler method is to
	//substitute the root.
 	raw_data syndromes = get_factorised_generator(t);
	//loop replaces factors with syndrome generated by that factor
	for(unsigned int i = 0; i < syndromes.length; i++)
	{
		//the accumulator is for adding the terms together
		uint8_t accumulator = 0;
		//calculating x^n by multiplying one at a time and doing a reverse loop
		//is more efficient than throwing away intermediate results
		uint8_t root_power_n = 1;
		for(int j = rd.length - 1; j >= 0; j--)
		{
			accumulator ^= galois_multiply(root_power_n, rd.data[j]);
			//root_power_n is multiplied after because first factor is x^0 = 1
			root_power_n = galois_multiply(root_power_n, syndromes.data[i]);
		}
		syndromes.data[i] = accumulator;
	}

	printf("syndromes:\n");
	for(unsigned int i = 0; i < syndromes.length; i++)
	{
		printf("%x\n", syndromes.data[i]);
	}

	/*
		uses syndrome_vector = syndrome_matrix X error_locator
		eg:
			( S2 )  =  ( S1  S0 ) X ( lambda0 )
			( S3 )     ( S2  S1 )   ( lambda1 )
	*/
	int v;
	uint8_t det = 0;
	raw_data syndrome_matrix;
	for(v = t; v > 0; v--)
	{
		syndrome_matrix.length = pow(v, 2);
		syndrome_matrix.data = (uint8_t*)alloc_named(syndrome_matrix.length, "rs_decode syndrome_matrix");

		//i is row, j is column, construct syndrome matrix
		for(unsigned int i = 0; i < v; i++)
		{
			for(unsigned int j = 0; j < v; j++)
			{
				syndrome_matrix.data[i * v + j] = syndromes.data[v-1 + i - j];
			}
		}

		//store in det so don't need to recalculate later for inverse
		det = determinant(syndrome_matrix);
		if(det == 0)  //fewer errors, keep trying
		{
			dealloc(syndrome_matrix.data);
		}
		else
		{
			printf("message has %i symbol errors!\n", v);
			break;
		}
	}

	raw_data syndrome_vector;
	syndrome_vector.length = v;
	syndrome_vector.data = (uint8_t*)alloc_named(syndrome_vector.length, "rs_decode syndrome_vector");
	for(unsigned int i = v; i < 2 * v; i++)
		syndrome_vector.data[i - v] = syndromes.data[i];

	printf("syndrome matrix:\n");
	print_matrix(syndrome_matrix);

	raw_data syndrome_matrix_inv = inverse(syndrome_matrix, det);

	printf("inverse matrix:\n");
	print_matrix(syndrome_matrix_inv);

	raw_data error_locator = mat_vec_multiply(syndrome_matrix_inv, syndrome_vector);

	printf("error_locator:\n");
	for(unsigned int i = 0; i < error_locator.length; i++)
	{
		printf("%x\n", error_locator.data[i]);
	}

	raw_data error_positions;
	error_positions.length = v;
	error_positions.data = (uint8_t*)alloc_named(error_positions.length, "rs_decode error_positions");

	//sub in all possible values of x in lambda(x) (Chein search)
	uint8_t errors_found = 0;
	for(unsigned int x = 0; x < 256; x++)
	{
		uint8_t accumulator = 1;  //lambda(x) equation starts with 1
		uint8_t x_pow_n = 1;
		for(unsigned int n = 0; n < error_locator.length; n++)
		{
			x_pow_n = galois_multiply(x_pow_n, x);  //multiply by x each time
			accumulator ^= galois_multiply(error_locator.data[n], x_pow_n);
		}
		if(accumulator == 0)
		{
			error_positions.data[errors_found] = rd.length - GF256inv(galois_divide(1, x));
			errors_found++;
			if(errors_found == v)
				break;
		}
	}

	printf("error positions:\n");
	for(unsigned int i = 0; i < error_positions.length; i++)
		printf("%i\n", error_positions.data[i]);

	//construct the locator matrix from the error locator polynomial
	raw_data locator_matrix;
	locator_matrix.length = pow(error_locator.length, 2);
	locator_matrix.data = (uint8_t*)alloc_named(locator_matrix.length, "rs_decode locator_matrix");

	raw_data error_locator_pow;
	error_locator_pow.length = error_locator.length;
	error_locator_pow.data = (uint8_t*)alloc_named(error_locator_pow.length, "error_locator_pow");

	for(unsigned int j = 0; j < v; j++)
		error_locator_pow.data[j] = 1; //galois_multiply(error_locator.data[j], error_locator_pow.data[j]);

	printf("error_locator_pow:\n");
	for(unsigned int i = 0; i < error_locator_pow.length; i++)
		printf("%x ", error_locator_pow.data[i]);
	printf("\n");

	memcpy(error_locator.data, error_positions.data, error_locator.length);
	for(unsigned int i = 0; i < error_locator.length; i++)
	{
		error_locator.data[i] = GF256(rd.length - error_locator.data[i]);
		//error_lcoator.data[i] =
	}

	printf("new error_locator:\n");
	for(unsigned int i = 0; i < error_locator.length; i++)
		printf("%x\n", error_locator.data[i]);

	//use cumulative multiplication for powers of other rows
	for(unsigned int i = 0; i < v; i++)
	{
		for(unsigned int j = 0; j < v; j++)
		{
			*ptr_at(locator_matrix, i, j) = error_locator_pow.data[j];
			error_locator_pow.data[j] = galois_multiply(error_locator_pow.data[j], error_locator.data[j]);
		}
	}

	//shorten "syndromes" vector to length v
	/*if(syndromes.length > v)
	{
		uint8_t* new_syndromes = (uint8_t*)alloc_named(v, "rs_decode new_syndromes");
		memcpy(new_syndromes, syndromes.data, v);
		dealloc(syndromes.data);
		syndromes.data = new_syndromes;
		syndromes.length = v;
	}*/
	syndromes.length = v;

	printf("shortened syndrome:\n");
	for(unsigned int i = 0; i < syndromes.length; i++)
		printf("%x\n", syndromes.data[i]);

	printf("locator_matrix:\n");
	print_matrix(locator_matrix);

	raw_data locator_inverse = inverse(locator_matrix, determinant(locator_matrix));

	printf("locator_inverse:\n");
	print_matrix(locator_inverse);

	raw_data value_vector = mat_vec_multiply(locator_inverse, syndromes);

	printf("error_values:\n");
	for(unsigned int i = 0; i < value_vector.length; i++)
		printf("%x\n", value_vector.data[i]);

	ret.length = rd.length - 2 * t;
	printf("ret.length = %i\n", ret.length);
	ret.data = (uint8_t*)alloc_named(ret.length, "rs_decode ret");
	memcpy(ret.data, rd.data, ret.length);

	for(unsigned int i = 0; i < value_vector.length; i++)
	{
		if(error_positions.data[i] < ret.length)
			ret.data[error_positions.data[i]] ^= value_vector.data[i];
	}

  return ret;
}
