//This file contains facilities for testing the API, which is in other files.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "packet.h"
#include "datatypes.h"
#include "hamming.h"

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

char insert_error(char input, int position)
{
	return (input ^ (1 << position));
}

void insert_errors(char* block, int length, int num_errors)
{
	//currently no protection against flipping the same bit twice, so should not be used to test bit error rates yet.
  int* error_positions = (int*)malloc(num_errors * sizeof(int));
	for (unsigned int i = 0; i < num_errors; i++)
	{
		int error_position = random(0, length * 8);
    while(1)
    {
      int j;
      for(j = 0; j < i; j++)
      {
        if(error_positions[j] == error_position)
          error_position = random(0, length * 8);
      }
      if(j == i)  //all tested, no errors are at same position
        break;
    }
		block[error_position / 8] = insert_error(block[error_position / 8], error_position % 8);
	}
  free(error_positions);
}

float pass_rate(int num_tests, int num_errors)
{
  int successes = 0;
  for(unsigned int i = 0; i < num_tests; i++)
  {
    for(unsigned int i = 0; i < 64; i++)
      test_data[i] = rand();

      raw_data rd;
      rd.length = 64;
      rd.data = test_data;
      packet p, q;

      //make packet
      p = make_packet(rd);

      //encode packet (using Hamming)
      encoded_packet ep = encode(&p);

      //simulate lossy trasmission and receiving
      insert_errors(ep.data, ep.length, num_errors);

      //decode received data
      q = decode(ep, NULL);

      free(ep.data);

      if(!memcmp(&p, &q, sizeof(packet)))
        successes++;
  }
  return (float)successes / (float)num_tests;
}

int main()
{
  srand(time(NULL));

  for(int errors = 0; errors <= 100; errors++)
  {
    printf("%i,%f\n", errors, pass_rate(10000, errors));
  }
  return 0;
}
