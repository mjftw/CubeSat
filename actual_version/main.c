//This file contains facilities for testing the API, which is in other files.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "packet.h"
#include "datatypes.h"
#include "hamming.h"
#include "interleave.h"
#include "bitstream.h"
#include "convolute.h"
#include "memory_tracker.h"

uint8_t test_data[64];

void printx(raw_data rd)
{
  for(unsigned int i = 0; i < rd.length; i++)
  {
    printf("%x ", rd.data[i]);
  }
}

void printp(const packet* p)
{
  for(unsigned int i = 0; i < sizeof(packet); i++)
  {
    printf("%x ", ((uint8_t*)p)[i]);
  }
}

int random(int min, int max)
{
	return rand() % (max - min) + min;
}

//say fraction = 1/2 returns 1 half the time and 0 half the time
int chance(float fraction)
{
  return ((float)rand() / RAND_MAX) < fraction;
}

char insert_error(char input, int position)
{
	return (input ^ (1 << position));
}

void insert_errors(char* block, int length, int num_errors)
{
  //int* error_positions = (int*)malloc(num_errors * sizeof(int));
	for (unsigned int i = 0; i < num_errors; i++)
	{
		int error_position = random(0, length * 8);
    /*while(1)
    {
      int j;
      for(j = 0; j < i; j++)
      {
        if(error_positions[j] == error_position)
          error_position = random(0, length * 8);
      }
      if(j == i)  //all tested, no errors are at same position
      {
        printf("break\n");
        break;
      }
    }*/
		block[error_position / 8] = insert_error(block[error_position / 8], error_position % 8);
	}
  //free(error_positions);
}

//does the same as insert errors, but has a half chance of inserting one of the
//remaining errors next to an existing one
void insert_errors_dependent(char* block, int length, int num_errors)
{
  //int* error_positions = (int*)malloc(num_errors * sizeof(int));
  int last_error_position = random(0, length * 8);
  for (unsigned int i = 0; i < num_errors; i++)
  {
    int error_position;  //half chance of being inserted next to last error
    if(chance(1.0/2.0))
      error_position = last_error_position + 1;
    else
      error_position = random(0, length * 8);
    if(error_position >= length * 8)
      error_position = random(0, length * 8);
    /*while(1)
    {
      int j;
      for(j = 0; j < i; j++)
      {
        if(error_positions[j] == error_position)
          error_position = random(0, length * 8);
      }
      if(j == i)  //all tested, no errors are at same position
        break;
    }*/
    block[error_position / 8] = insert_error(block[error_position / 8], error_position % 8);
    last_error_position = error_position;
  }
  //free(error_positions);
}

//error inserter is a function to insert errors, so it's easy to change out
float pass_rate(int num_tests, int num_errors, void (*error_inserter)(char*, int, int), int interleave_data)
{
  int successes = 0;
  for(unsigned int i = 0; i < num_tests; i++)
  {
    for(unsigned int i = 0; i < 64; i++)
      test_data[i] = 255;

    raw_data rd;
    rd.length = 64;
    rd.data = test_data;
    packet p, q;

    //make packet
    p = make_packet(rd);

    //encode packet (using Hamming)
    encoded_packet ep = encode(&p);

    if(interleave_data)
      interleave(ep);

    //simulate lossy trasmission and receiving
    error_inserter(ep.data, ep.length, num_errors);

    //decode received data
    if(interleave_data)
      deinterleave(ep);

    q = decode(ep, NULL);
    dealloc(ep.data);

    if(!memcmp(&p, &q, sizeof(packet)))
      successes++;
  }
  return (float)successes / (float)num_tests;
}

int main()
{
  //this part tests memory_tracker
  /*int* a[100];
  for(unsigned int i = 0; i < 100; i++)
  {
    a[i] = (int*)alloc(rand() % 100);
    printf("total_allocated_space = %i\n", allocated());
    printf("peak_allocated_space = %i\n", peak_allocated());
  }
  for(unsigned int i = 0; i < 100; i++)
  {
    dealloc(a[i]);
    printf("total_allocated_space = %i\n", allocated());
    printf("peak_allocated_space = %i\n", peak_allocated());
  }

  return 0;*/


  //this part tests convolution
  /*raw_data rd;
  int starting_length = 5;
  rd.length = starting_length;
  rd.data = (uint8_t*)malloc(starting_length);
  for(unsigned int i = 0; i < starting_length; i++)
    rd.data[i] = 0;
  rd.data[0] = 0xb0;

  for(unsigned int i = 0; i < rd.length; i++)
    printf("%x ", rd.data[i]);
  printf("\n");

  raw_data returned = convolute(rd);

  for(unsigned int i = 0; i < returned.length; i++)
    printf("%x ", returned.data[i]);
  printf("\n");

  raw_data decoded = deconvolute(returned, NULL);

  for(unsigned int i = 0; i < returned.length; i++)
    printf("%x ", returned.data[i]);
  printf("\n");

  free(rd.data);
  free(returned.data);
  free(decoded.data);
  return 0;*/


  //This part tests bitstreams
  /*for(int j = 0; j < 100; j++)
  {
    uint8_t* data = (uint8_t*)malloc(256);
    for(unsigned int i = 0; i < 256; i++)
      data[i] = i;

    uint8_t* data2 = (uint8_t*)malloc(256);
    for(unsigned int i = 0; i < 256; i++)
      data2[i] = 0;

    int position = 0, position2 = 0;
    for(unsigned int i = 0; position < 255 * 8; i++)
    {
      int check_size = rand() % 8 + 1;
      uint8_t bits = get_bits_from_position(data, check_size, &position);
      insert_bits_at_position(data2, bits, check_size, &position2);
    }

    //printf("check_size = %i\n", check_size);
    if(!memcmp(data, data2, 128))
    {
    }
    else
    {
      printf("failure\n");
      for(unsigned int i = 0; i < 256; i++)
        printf("%x ", data2[i]);
    }
    //printf("\n\n");

    free(data);
    free(data2);
  }

  //for(unsigned int i = 0; i < 256; i++)
  //  printf("%x ", data2[i]);
  //printf("\n");

  return 0;*/


  //This part tests interleaving
  /*char message[50];
  char check_message[50];
  for(unsigned int i = 0; i < 50; i++)
    message[i] = rand();
  memcpy(check_message, message, 50);

  raw_data rd;
  rd.length = 50;
  rd.data = message;

  for(unsigned int i = 0; i < 50; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  interleave(rd);

  for(unsigned int i = 0; i < 50; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  deinterleave(rd);

  for(unsigned int i = 0; i < 50; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  if(!memcmp(message, check_message, 50))
    printf("success!");
  else
    printf("Failure!");
  return 0;*/

  srand(time(NULL));

  printf("num_errors,MPR_independent,MPR_dependent,MPR_independent_interleave,MPR_dependent_interleave\n");
  for(int errors = 0; errors <= 10; errors++)
  {
    printf("%i,%f,%f,%f,%f\n", errors,
      pass_rate(1000, errors, insert_errors, 0),
      pass_rate(1000, errors, insert_errors_dependent, 0),
      pass_rate(1000, errors, insert_errors, 1),
      pass_rate(1000, errors, insert_errors_dependent, 1));
  }
  print_memory_usage_stats();
  return 0;
}
