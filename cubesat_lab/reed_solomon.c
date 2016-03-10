#include "reed_solomon.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "datatypes.h"
#include "memory_tracker.h"
#include "galois_field.h"
#include "matrix.h"

//defines how many generator polynomials will be remembered so they
//don't need to be recalculated
#define GENERATOR_MEMORY 2

//copy required should be 1 if the returned value is to be modified.
//any copies returned need to be dealloced, non-copys must not be.
//will be faster if no copy is required. If GENERATOR_MEMORY == 0,
//all data returned will be need to be dealloced.
raw_data get_factorised_generator(int t, uint8_t copy_required)
{
#if GENERATOR_MEMORY > 0
	static int t_memory[GENERATOR_MEMORY] = {0};  //initialise all to 0
	static raw_data g_memory[GENERATOR_MEMORY];
	static int next_RR = 0;  //round robin replacement strategy

	if(next_RR == GENERATOR_MEMORY)
		next_RR = 0;

	unsigned int i;
	for(i = 0; i < GENERATOR_MEMORY; i++)
	{
		if(t == t_memory[i])
		{
			next_RR = i+1;
			if(!copy_required)
				return g_memory[i];
			else
			{
				raw_data ret;
				ret.length = g_memory[i].length;
				ret.data = (uint8_t*)alloc_named(ret.length, "get_factorised_generator ret.data");
				memcpy(ret.data, g_memory[i].data, ret.length);
				next_RR++;
				return ret;
			}
		}
	}

	if(t_memory[next_RR] != 0)
		dealloc_named(g_memory[next_RR].data, "get_factorised_generator g_memory[next_RR].data");

	g_memory[next_RR].length = 2 * t;
	g_memory[next_RR].data = (uint8_t*)alloc_named(g_memory[next_RR].length, "get_factorised_generator g_memory[next_RR].data");

	for(i = 0; i < g_memory[next_RR].length; i++)
		g_memory[next_RR].data[i] = GF256(i+1);
	t_memory[next_RR] = t;

	if(!copy_required)
		return g_memory[next_RR++];
	else
	{
		raw_data ret;
		ret.length = g_memory[next_RR].length;
		ret.data = (uint8_t*)alloc_named(ret.length, "get_factorised_generator ret.data");
		memcpy(ret.data, g_memory[next_RR].data, ret.length);
		next_RR++;
		return ret;
	}
#else
	raw_data ret;
	ret.length = 2 * t;
	ret.data = (uint8_t*)alloc_named(ret.length, "get_factorised_generator factorised");
	unsigned int i;
	for(i = 0; i < ret.length; i++)
		ret.data[i] = GF256(i+1);

	return ret;
#endif
}

raw_data produce_generator_polynomial(int t)
{
	unsigned int i;
#if GENERATOR_MEMORY > 0
	static int t_memory[GENERATOR_MEMORY] = {0};  //initialise all to 0
	static raw_data g_memory[GENERATOR_MEMORY];
	static int next_RR = 0;  //round robin replacement strategy

	if(next_RR == GENERATOR_MEMORY)
		next_RR = 0;

	for(i = 0; i < GENERATOR_MEMORY; i++)
	{
		if(t == t_memory[i])
		{
			next_RR = i+1;
			return g_memory[i];
		}
	}

	if(t_memory[next_RR] != 0)
		dealloc_named(g_memory[next_RR].data, "produce_generator_polynomial g_memory[next_RR].data");

	raw_data* ret = &(g_memory[next_RR]);
#else
	raw_data actual_ret;
	raw_data* ret = &actual_ret;
#endif  //GENERATOR_MEMORY > 0

	//start with factors like (x+GF256(1)(x+GF256(2)) etc
	//expand factors into each other
	raw_data factorised = get_factorised_generator(t, 0);

	raw_data aux;
	ret->length = 2 * t + 1;
	ret->data = (uint8_t*)alloc_named(ret->length, "produce_generator_polynomial ret->data");
	aux.length = 2 * t + 1;
	aux.data = (uint8_t*)alloc_named(aux.length, "produce_generator_polynomial aux.data");

	//ret and aux represent polynomials. The index represents the power and the
	//value represents the coefficient. aux is auxiliary for shift and add
	ret->data[0] = 1;
	for(i = 1; i < ret->length; i++)
	{
		ret->data[i] = 0;
	}

	for(i = 0; i < factorised.length; i++)
	{
		//multiply - put result in aux
		unsigned int j;
		for(j = 0; j < ret->length; j++)
			aux.data[j] = galois_multiply(ret->data[j], factorised.data[i]);

		//shift in place
		for(j = ret->length - 1; j > 0; j--)
			ret->data[j] = ret->data[j-1];
		ret->data[0] = 0;

		//add
		for(j = 0; j < ret->length; j++)
			ret->data[j] ^= aux.data[j];
	}

	dealloc_named(aux.data, "produce_generator_polynomial aux.data");
#if GENERATOR_MEMORY == 0
	dealloc_named(factorised.data, "produce_generator_polynomial factorised.data");
#endif
	//reverse in place
	for(i = 0; i < ret->length / 2; i++)
	{
		uint8_t tmp = ret->data[i];
		ret->data[i] = ret->data[ret->length - 1 - i];
		ret->data[ret->length - 1 - i] = tmp;
	}
#if GENERATOR_MEMORY > 0
	t_memory[next_RR] = t;
	next_RR++;
#endif
	return *ret;
}

//the encode function simply calculates the 2t parity checking symbols and returns T(x)
raw_data rs_encode(raw_data rd, int t)
{
  raw_data ret;

	ret.length = rd.length + 2 * t;
	ret.data = (uint8_t*)alloc_named(ret.length, "rs_encode ret");

	memcpy(ret.data, rd.data, rd.length);  //copy message
	unsigned int i;
	for(i = rd.length; i < ret.length; i++)
		ret.data[i] = 0;  //initialise parity symbols to 0

	if(t == 0)
		return ret;

	raw_data g_x = produce_generator_polynomial(t);

	//do division here-
	for(i = 0; i < ret.length - 2 * t; i++)
	{
		if(ret.data[i] == 0)  //multiply yields 0, so subtract doesn't change it
			continue;
		uint8_t multiplier = ret.data[i];
		unsigned int j;
		for(j = 0; j < g_x.length; j++)
			ret.data[i+j] ^= galois_multiply(g_x.data[j], multiplier);
	}

	//copy message back- as message symbols are unchanged in T(x)
	memcpy(ret.data, rd.data, rd.length);

#if GENERATOR_MEMORY == 0
	dealloc_named(g_x.data, "rs_encode g_x.data");
#endif

  return ret;
}

uint8_t rs_decode(raw_data rd, raw_data* ret, int t, int* bit_error_count)
{
	if(t == 0)
	{
		ret->length = rd.length;
		ret->data = (uint8_t*)alloc_named(ret->length, "rs_decode ret->data");
		memcpy(ret->data, rd.data, ret->length);
		return 1;
	}
	//calculate syndromes: this is done by dividing the received polynomial by
	//all the factors of the generator polynomial. The simpler method is to
	//substitute the root.
 	raw_data syndromes = get_factorised_generator(t, 1);
	//loop replaces factors with syndrome generated by that factor

	uint8_t all_zeroes = 1;
	unsigned int i;
	for(i = 0; i < syndromes.length; i++)
	{
		//the accumulator is for adding the terms together
		uint8_t accumulator = 0;
		//calculating x^n by multiplying one at a time and doing a reverse loop
		//is more efficient than throwing away intermediate results
		uint8_t root_power_n = 1;
		int j;
		for(j = rd.length - 1; j >= 0; j--)
		{
			accumulator ^= galois_multiply(root_power_n, rd.data[j]);
			//root_power_n is multiplied after because first factor is x^0 = 1
			root_power_n = galois_multiply(root_power_n, syndromes.data[i]);
		}
		syndromes.data[i] = accumulator;
		if(accumulator != 0)
			all_zeroes = 0;
	}

	//if all syndromes are 0, no errors have occurred.
	if(all_zeroes)
	{
		ret->length = rd.length - 2 * t;
		ret->data = (uint8_t*)alloc_named(ret->length, "rs_decode ret");
		memcpy(ret->data, rd.data, ret->length);
		dealloc_named(syndromes.data, "rs_decode syndromes.data");
		return 1;
	}

	/*uses syndrome_vector = syndrome_matrix X error_locator
		eg:
			( S2 )  =  ( S1  S0 ) X ( lambda0 )
			( S3 )     ( S2  S1 )   ( lambda1 )  */
	int v;
	uint8_t det = 0;
	raw_data syndrome_matrix;
	for(v = t; v > 0; v--)
	{
		syndrome_matrix.length = pow(v, 2);
		syndrome_matrix.data = (uint8_t*)alloc_named(syndrome_matrix.length, "rs_decode syndrome_matrix");

		//i is row, j is column, construct syndrome matrix
		unsigned int j;
		for(i = 0; i < v; i++)
			for(j = 0; j < v; j++)
				syndrome_matrix.data[i * v + j] = syndromes.data[v-1 + i - j];

		//store in det so don't need to recalculate later for inverse
		det = determinant(syndrome_matrix);
		if(det == 0)  //fewer errors, keep trying
			dealloc_named(syndrome_matrix.data, "rs_decode syndrome_matrix.data");
		else
			break;
	}
	if(v == 0)
	{
		dealloc_named(syndromes.data, "rs_decode syndromes.data 2");
		ret->length = rd.length - 2 * t;
		ret->data = alloc_named(ret->length, "rs_decode ret.data");
		memcpy(ret->data, rd.data, ret->length);
		return 0;
	}

	raw_data syndrome_vector;
	syndrome_vector.length = v;
	syndrome_vector.data = (uint8_t*)alloc_named(syndrome_vector.length, "rs_decode syndrome_vector");
	for(i = v; i < 2 * v; i++)
		syndrome_vector.data[i - v] = syndromes.data[i];

	raw_data syndrome_matrix_inv = inverse(syndrome_matrix, det);

	raw_data error_locator = mat_vec_multiply(syndrome_matrix_inv, syndrome_vector);

	raw_data error_positions;
	error_positions.length = v;
	error_positions.data = (uint8_t*)alloc_named(error_positions.length, "rs_decode error_positions");

	//sub in all possible values of x in lambda(x) (Chien search)
	uint8_t errors_found = 0;
	unsigned int x;
	for(x = 0; x < 256; x++)
	{
		uint8_t accumulator = 1;  //lambda(x) equation starts with 1
		uint8_t x_pow_n = 1;
		unsigned int n;
		for(n = 0; n < error_locator.length; n++)
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

	//construct the locator matrix from the error locator polynomial
	raw_data locator_matrix;
	locator_matrix.length = pow(error_locator.length, 2);
	locator_matrix.data = (uint8_t*)alloc_named(locator_matrix.length, "rs_decode locator_matrix");

	raw_data error_locator_pow;
	error_locator_pow.length = error_locator.length;
	error_locator_pow.data = (uint8_t*)alloc_named(error_locator_pow.length, "error_locator_pow");

	int j;
	for(j = 0; j < v; j++)
		error_locator_pow.data[j] = 1; //galois_multiply(error_locator.data[j], error_locator_pow.data[j]);

	memcpy(error_locator.data, error_positions.data, error_locator.length);
	for(i = 0; i < error_locator.length; i++)
		error_locator.data[i] = GF256(rd.length - error_locator.data[i]);

	//use cumulative multiplication for powers of other rows
	for(i = 0; i < v; i++)
	{
		for(j = 0; j < v; j++)
		{
			*ptr_at(locator_matrix, i, j) = error_locator_pow.data[j];
			error_locator_pow.data[j] = galois_multiply(error_locator_pow.data[j], error_locator.data[j]);
		}
	}

	//shorten syndromes vector. No need to reallocate
	//as dealloc doesn't care about size
	syndromes.length = v;

	//calculate inverse of locator matrix
	raw_data locator_inverse = inverse(locator_matrix, determinant(locator_matrix));

	//multiply syndromes by inverse matrix to get error value vector
	raw_data value_vector = mat_vec_multiply(locator_inverse, syndromes);

	//allocate ret and copy message
	//need to allocate space for parity bits too
	//so these can be corrected, and used to determine whether correction worked.
	ret->length = rd.length;
	ret->data = (uint8_t*)alloc_named(ret->length, "rs_decode ret");
	memcpy(ret->data, rd.data, ret->length);

	//correct errors in copied code
	uint8_t corrected = 2;
	for(i = 0; i < value_vector.length; i++)
		if(error_positions.data[i] < ret->length)
			ret->data[error_positions.data[i]] ^= value_vector.data[i];
		else
			corrected = 0;

	if(corrected && v == t)
	{
		//redo calculation of syndromes divided by decoded code word
		//syndromes should all be 0 if decoding is correct, otherwise decoding is not correct
		dealloc_named(syndromes.data, "rs_decode syndromes.data 2");
		uint8_t all_zeroes = 1;
		syndromes = get_factorised_generator(t, 1);
		for(i = 0; i < syndromes.length; i++)
		{
			//the accumulator is for adding the terms together
			uint8_t accumulator = 0;
			//calculating x^n by multiplying one at a time and doing a reverse loop
			//is more efficient than throwing away intermediate results
			uint8_t root_power_n = 1;
			for(j = ret->length - 1; j >= 0; j--)
			{
				accumulator ^= galois_multiply(root_power_n, ret->data[j]);
				//root_power_n is multiplied after because first factor is x^0 = 1
				root_power_n = galois_multiply(root_power_n, syndromes.data[i]);
			}
			syndromes.data[i] = accumulator;
			if(accumulator != 0)
				all_zeroes = 0;
		}

		//if all syndromes are 0, no errors have occurred.
		if(all_zeroes)
			corrected = 3;
		else
			corrected = 0;
	}

	//dealloc all alloced data including that which was returned from functions
	//called to implement rs_decode
	dealloc_named(locator_inverse.data, "rs_decode locator_inverse.data");
	dealloc_named(value_vector.data, "rs_decode value_veector.data");
	dealloc_named(locator_matrix.data, "rs_decode locator_matrix.data");
	dealloc_named(error_locator_pow.data, "rs_decode error_locator_pow.data");
	dealloc_named(error_locator.data, "rs_decode error_locator.data");
	dealloc_named(error_positions.data, "rs_decode error_positions.data");
	dealloc_named(syndrome_matrix_inv.data, "rs_decode syndrome_matrix_inv.data");
	dealloc_named(syndrome_matrix.data, "rs_decode syndrome_matrix.data 2");
	dealloc_named(syndrome_vector.data, "rs_decode syndrome_vector.data");
	dealloc_named(syndromes.data, "rs_decode syndromes.data 3");

	ret->length -= 2 * t;
  return corrected;
}
