//This file contains facilities for testing the API, which is in other files.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "packet.h"
#include "datatypes.h"
#include "hamming.h"
#include "interleave.h"
#include "bitstream.h"
#include "convolute.h"
#include "memory_tracker.h"
#include "reed_solomon.h"
#include "matrix.h"
#include "galois_field.h"

void printx(raw_data rd)
{
  unsigned int i;
  for(i = 0; i < rd.length; i++)
  {
    printf("%x ", rd.data[i]);
  }
}

void printp(const packet* p)
{
  unsigned int i;
  for(i = 0; i < sizeof(packet); i++)
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

void insert_errors(uint8_t* block, int length, int num_errors)
{
  //int* error_positions = (int*)malloc(num_errors * sizeof(int));
  unsigned int i;
	for (i = 0; i < num_errors; i++)
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
void insert_errors_dependent(uint8_t* block, int length, int num_errors)
{
  //int* error_positions = (int*)malloc(num_errors * sizeof(int));
  int last_error_position = random(0, length * 8);
  unsigned int i;
  for (i = 0; i < num_errors; i++)
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

int num_errors_inserted = 0;

void insert_errors2(uint8_t* block, int length, float BER)
{
  unsigned int i;
  for(i = 0; i < length; i++)
  {
    unsigned int j;
    for(j = 0; j < 8; j++)
    {
      if(chance(BER))
      {
        block[i] = insert_error(block[i], j);
        num_errors_inserted++;
      }
    }
  }
}

//counts the symbol errors and returns 1 if symbol errors <= t
int is_correctible(raw_data a, raw_data b, int t, int show_se)
{
  assert(a.length = b.length);
  int symbol_errors = 0;
  unsigned int i;
  for(i = 0; i < a.length; i++)
  {
    //any symbol errors which exist are in the parity bits- correctible no matter how many there are
    if((i == a.length - 2*t) && symbol_errors == 0)
      return 1;
    if(a.data[i] != b.data[i])
    {
      symbol_errors++;
      if(show_se)
        printf("position = %i\n", i);
    }
  }
  if(show_se)
    printf("%i\n", symbol_errors);
  return symbol_errors <= t;
}

//uses is_correctible instead of the actual reed_solomon codec to make it faster
float message_pass_rate_sim(int num_errors_min, int tries_max, float BER, int t)
{
  int successes = 0;
  int errors = 0;
  int tries = 0;

  while(errors < num_errors_min)
  {

    raw_data rd;
    rd.length = 128 + 2*t;
    rd.data = (uint8_t*)alloc_named(rd.length, "message_pass_rate_sim rd");
    unsigned int i;
    for(i = 0; i < rd.length; i++)
      rd.data[i] = rand();

    raw_data ep = convolute(rd);

    //interleaving not needed as errror insertion is AWGN- faster to not do it
    //interleave(ep);  //interleaves data in place

    insert_errors2(ep.data, ep.length, BER);

    //deinterleave(ep);

    raw_data deconvoluted = deconvolute(ep, NULL);

    if(is_correctible(rd, deconvoluted, t, 0))
    {
      successes++;
    }
    else
    {
      errors++;
    }

    dealloc(ep.data);
    dealloc(deconvoluted.data);
    dealloc(rd.data);
    if(tries >= tries_max)
      break;

    tries++;
  }
  return (float)successes / ((float)successes + (float)errors);
}

//tests the methods used encoding and decoding, uses insert_errors2 to accomodate any bit rate
float message_pass_rate(int num_tests, float BER)
{
  int successes = 0;
  int errors = 0;
  int tries = 0;
  int wrong1 = 0, wrong2 = 0;
  //for(unsigned int i = 0; i < num_tests; i++)
  while(errors < 100)
  {

    raw_data rd;
    rd.length = 64;
    rd.data = (uint8_t*)alloc_named(rd.length, "message_pass_rate rd");
    unsigned int i;
    for(i = 0; i < 64; i++)
      rd.data[i] = rand();

    int t = 2;  //PYTHON//

    //p = make_packet(rd);
    //encoded_packet ep = encode(&p);  //currently convolutional- subject to change

    raw_data rs_encoded = rs_encode(rd, t);
    raw_data ep = convolute(rs_encoded);

    interleave(ep);  //interleaves data in place

    insert_errors2(ep.data, ep.length, BER);

    deinterleave(ep);

    raw_data deconvoluted = deconvolute(ep, NULL);

    int isc = is_correctible(rs_encoded, deconvoluted, t, 0);

    raw_data decoded;
    rs_decode(deconvoluted, &decoded, t, NULL);

    //q = decode(ep, NULL);
    if(rd.length != decoded.length)
    {
      printf("lengths are not the same!\n");
      printf("rd.length = %i, decoded.length = %i\n", rd.length, decoded.length);
    }
    if(!memcmp(decoded.data, rd.data, rd.length))
    {
      if(!isc)
        wrong1++;
      //if(!isc)
      //  is_correctible(rs_encoded, deconvoluted, t, 1);
      //assert(isc);
      successes++;
    }
    else
    {
      if(isc)
        wrong2++;
      //assert(!isc);
      errors++;
    }

    dealloc(ep.data);
    dealloc(rs_encoded.data);
    dealloc(deconvoluted.data);
    dealloc(decoded.data);
    dealloc(rd.data);
    if(tries >= num_tests)
    {
      printf("tries = %i, errors = %i\n", tries, errors);
      break;
    }
    tries++;
  }
  //return (float)successes / (float)num_tests;
  printf("wrong1 = %i\n", wrong1);
  printf("wrong2 = %i\n", wrong2);
  printf("tests = %i\n", successes + errors);
  return (float)successes / ((float)successes + (float)errors);
}

float time_function()
{
  time_t now = time(NULL);
  unsigned int runs = 0;
  unsigned int time_length = 5;  //seconds
  while(time(NULL) == now);
  now++;
  int t = 2;

  //setup for funciton here
  raw_data rd;
  rd.length = 64;
  rd.data = (uint8_t*)alloc_named(rd.length, "time_function rd.data");

  raw_data rd2 = packet_data(rd, t, 4);
  insert_errors2(rd2.data, rd2.length, 0.1442);
  raw_data rd3;

  while(time(NULL) - time_length < now)
  {
    unpacket_data(rd2, &rd3, t, 4, NULL);
    dealloc(rd3.data);
    runs++;
  }

  //setdown for function here
  dealloc(rd.data);

  return (float)runs / (float)time_length;
}

typedef struct
{
  unsigned int length;
  float* data;
} table_t;

table_t read_snr_ber()
{
  FILE* snr_ber = fopen("SNR_BER.csv", "r");
  fseek(snr_ber, 0, SEEK_END);
  unsigned int length = ftell(snr_ber);
  fseek(snr_ber, 0, SEEK_SET);

  char* buffer = (char*)malloc(length);
  fread(buffer, length, 1, snr_ber);

  unsigned int lines = 0;
  unsigned int i;
  for(i = 0; i < length; i++)
    if(buffer[i] == '\n')
      lines++;

  float* table = (float*)malloc(lines * sizeof(float) * 2);

  char tmp[16];
  int j = 0;
  int table_index = 0;
  for(i = 0; i < length; i++)
  {
    if(buffer[i] == ',' || buffer[i] == '\n')
    {
      tmp[j] = '\0';
      table[table_index++] = atof(tmp);
      j = 0;
    }
    else
      tmp[j++] = buffer[i];
  }
  free(buffer);
  fclose(snr_ber);
  table_t ret;
  ret.length = lines;
  ret.data = table;
  return ret;
}

float table_lookup(table_t* table, float value)
{
  unsigned int i;
  for(i = 2; i < table->length * 2; i += 2)
  {
    //printf("%f\n", table->data[i]);
    if(table->data[i] >= value && table->data[i-2] < value)
    {
      return table->data[i-1];
    }
  }
  return 0;
}

float reverse_table_lookup(table_t* table, float value)
{
  unsigned int i;
  for(i = 1; i < table->length * 2 - 1; i += 2)
  {
    //printf("%f\n", table->data[i]);
    if(table->data[i] <= value && table->data[i-2] > value)
    {
      return table->data[i-1];
    }
  }
  return 0;
}

float coding_gain(float SNR, int t, int tests)
{
  table_t snr_ber = read_snr_ber();
  float BER = table_lookup(&snr_ber, SNR);
  printf("channel BER = %f\n", BER);

  unsigned int errors = 0;
  unsigned int i;
  for(i = 0; i < tests; i++)
  {
    raw_data rd;
    rd.length = 64;
    rd.data = (uint8_t*)alloc_named(rd.length, "message_pass_rate rd");
    unsigned int j;
    for(j = 0; j < 64; j++)
      rd.data[j] = rand();

    raw_data encoded_packet = packet_data(rd, t, 16);
    insert_errors2(encoded_packet.data, encoded_packet.length, BER);
    raw_data decoded_packet;
    unpacket_data(encoded_packet, &decoded_packet, t, 16, NULL);

    for(j = 0; j < rd.length; j++)
    {
      uint8_t tmp = rd.data[j] ^ decoded_packet.data[j];
      unsigned int k;
      for(k = 0; k < 8; k++)
      {
        if(1 << k & tmp)
          errors++;
      }
    }
  }
  printf("new BER = %f\n", (float)errors / (float)(64 * 8 * tests));
  return reverse_table_lookup(&snr_ber, (float)errors / (float)(64 * 8 * tests)) - SNR;
}

void test_packeting(unsigned int tries, unsigned int length, float BER, unsigned int rs_t, unsigned int conv_constraint)
{
  //tests packeting code.
  //Also tests for false positives and false negative decoding.
  int correct = 0, incorrect1 = 0, incorrect2 = 0;
  unsigned int i;
  for(i = 0; i < tries; i++)
  {
    raw_data rd;
    rd.length = length;
    rd.data = (uint8_t*)alloc_named(rd.length, "main rd.data");
    unsigned int j;
    for(j = 0; j < rd.length; j++)
      rd.data[j] = rand();

    raw_data packet = packet_data(rd, rs_t, conv_constraint);

    //insert errors here
    insert_errors2(packet.data, packet.length, BER);

    raw_data received;
    if(unpacket_data(packet, &received, rs_t, conv_constraint, NULL))
    {
      if(!memcmp(received.data, rd.data, rd.length))
        correct++;
      else
        incorrect1++;
    }
    else
    {
      if(memcmp(received.data, rd.data, rd.length))
        correct++;
      else
      {
        incorrect2++;
      }
    }

    dealloc(rd.data);
    dealloc(packet.data);
    dealloc(received.data);
  }
  printf("correct = %i\n", correct);
  printf("false positives = %i\n", incorrect1);
  printf("false negatives = %i\n", incorrect2);
}

void test_reed_solomon()
{
  //this part tests reed solomon coded_bits
  unsigned int false_positives = 0, false_negatives = 0;
  unsigned int tries = 10000;
  unsigned int correct = 0;
  unsigned int j;
  for(j = 0; j < tries; j++)
  {
    int t = 4;
    int error_count = 0;
    raw_data rd;
    rd.length = 64;
    rd.data = (uint8_t*)alloc_named(rd.length, "main rd.data");
    unsigned int i;
    for(i = 0; i < rd.length; i++)
      rd.data[i] = i;
    raw_data encoded = rs_encode(rd, t);

    uint8_t num_errors = rand() % (2 * t);
    for(i = 0; i < num_errors; i++)
      encoded.data[rand() % (rd.length + 2 * t)] = rand();

    raw_data decoded;
    uint8_t success = rs_decode(encoded, &decoded, t, &error_count);

    if(memcmp(rd.data, decoded.data, rd.length) && success)  //different but thinks it's right
    {
      false_positives++;
    }
    else if(!memcmp(rd.data, decoded.data, rd.length) && !success)  //same but thinks it's wrong
      false_negatives++;

    if(!memcmp(rd.data, decoded.data, rd.length))
       correct++;

    dealloc(rd.data);
    dealloc(encoded.data);
    dealloc(decoded.data);
  }
  printf("false_positives = %i\n", false_positives);
  printf("false_negatives = %i\n", false_negatives);
  printf("out of %i tries\n", tries);
  printf("percentage correct = %f\n", (float)(tries - (false_positives + false_negatives)) / (float)tries * 100.0);
  printf("num_correct = %i\n", correct);
}

void print_matrix(raw_data rd);  //gives access to private function in matrices.c

void test_matrices()
{
  //this part tests matrices
  raw_data mat;
  mat.length = 4;
  mat.data = (uint8_t*)alloc_named(mat.length, "main mat");

  mat.data[0] = 0xc2;
  mat.data[1] = 0xca;
  mat.data[2] = 0xf0;
  mat.data[3] = 0xd6;

  uint8_t test_data[2] = {2, 3};
  raw_data test;
  test.length = 2;
  test.data = test_data;

  printx(mat_vec_multiply(mat, test));

  unsigned int i;
  for(i = 0; i < mat.length; i++)
  {
    mat.data[i] = rand();
  }

  print_matrix(mat);
  printf("%x\n", determinant(mat));

  raw_data mat2 = inverse(mat, determinant(mat));

  print_matrix(mat2);

  dealloc(mat.data);
  dealloc(mat2.data);
}

void test_convolution()
{
  //this part tests convolution
  unsigned int j;
  int failed = 0;
  int iterations = 1000;
  for(j = 0; j < iterations; j++)
  {
    uint8_t data[128];
    unsigned int i;
    for(i = 0; i < 128; i++)
      data[i] = rand();

    raw_data rd;
    rd.data = data;
    rd.length = 128;
    raw_data received = convolute_constrained(rd, 16);

    insert_errors2(received.data, received.length, 0.02);

    raw_data decoded = deconvolute_constrained(received, NULL, 16);

    if(memcmp(rd.data, decoded.data, decoded.length))
    {
      failed++;
    }

    dealloc(received.data);
    dealloc(decoded.data);
  }
  printf("failure_rate = %f\n", (float)failed / (float)iterations);
}

void test_memory()
{
  //this part tests memory_tracker
  int* a[100];
  unsigned int i;
  for(i = 0; i < 100; i++)
  {
    a[i] = (int*)alloc(rand() % 100);
    printf("total_allocated_space = %i\n", allocated());
    printf("peak_allocated_space = %i\n", peak_allocated());
  }
  for(i = 0; i < 100; i++)
  {
    dealloc(a[i]);
    printf("total_allocated_space = %i\n", allocated());
    printf("peak_allocated_space = %i\n", peak_allocated());
  }
}

void test_bitstream()
{
  //This part tests bitstreams
  int j;
  for(j = 0; j < 100; j++)
  {
    uint8_t* data = (uint8_t*)malloc(256);
    unsigned int i;
    for(i = 0; i < 256; i++)
      data[i] = i;

    uint8_t* data2 = (uint8_t*)malloc(256);
    for(i = 0; i < 256; i++)
      data2[i] = 0;

    unsigned int position = 0, position2 = 0;
    for(i = 0; position < 255 * 8; i++)
    {
      int check_size = rand() % 8 + 1;
      uint8_t bits = get_bits_from_position(data, check_size, &position);
      insert_bits_at_position(data2, bits, check_size, &position2);
    }

    if(!memcmp(data, data2, 128))
    {
    }
    else
    {
      printf("failure\n");
      for(i = 0; i < 256; i++)
        printf("%x ", data2[i]);
    }

    free(data);
    free(data2);
  }
}

void test_interleaving()
{
  //This part tests interleaving
  uint8_t message[51];
  uint8_t check_message[51];
  unsigned int i;
  for(i = 0; i < 51; i++)
    message[i] = rand();
  memcpy(check_message, message, 51);

  raw_data rd;
  rd.length = 51;
  rd.data = message;

  for(i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  interleave(rd);

  for(i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  deinterleave(rd);

  for(i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  if(!memcmp(message, check_message, 51))
    printf("success!\n");
  else
    printf("Failure!\n");
}

void test_segmentation()
{
  printf("testing segmentation\n");
  unsigned int i;
  for(i = 0; i < 10000; i++)
  {
    raw_data rd;
    unsigned int segment_size = rand() % 10000;
    int failure = 0;
    rd.length = rand() % 10000;
    rd.data = alloc(rd.length);
    unsigned int j;
    for(j = 0; j < rd.length; j++)
      rd.data[j] = rand();
    raw_data* segments = NULL;
    unsigned int num_segments;
    segment_data(rd, segment_size, &segments, &num_segments);

    raw_data concatted = concatenate_segments(segments, num_segments);

    if(rd.length == concatted.length)
    {
      if(memcmp(rd.data, concatted.data, rd.length))
        failure = 1;
    }
    else
      failure = 1;
    if(failure)
    {
      printf("failure\n");
    }
    else
      //printf("success\n");

    dealloc(rd.data);
    dealloc(segments);
    dealloc(concatted.data);
  }
}

int main(int argc, char** argv)
{
  //purpose of main function- parse arguments and run max argv[1] tests or max errors argv[2] with BER of argv[3].
  //prints the message pass rate for these tests- to be interpreted by python.
  //many tests designed to run in parallel with different arguments
  srand(time(NULL));

  time_t start_time = time(NULL);

  if(argc >= 2)
  {
    if(!strcmp(argv[1],"packeting"))
    {
      //tries, length, BER, rs_t conv_constraint
      if(argc != 7)
        printf("Incorrect argument count for packeting.\n");
      else
        test_packeting(atoi(argv[2]), atoi(argv[3]), atof(argv[4]), atoi(argv[5]), atoi(argv[6]));
    }
    else if(!strcmp(argv[1], "memory"))
      test_memory();
    else if(!strcmp(argv[1], "matrices"))
      test_matrices();
    else if(!strcmp(argv[1], "bitstream"))
      test_bitstream();
    else if(!strcmp(argv[1], "convolution"))
      test_convolution();
    else if(!strcmp(argv[1], "reed_solomon"))
      test_reed_solomon();
    else if(!strcmp(argv[1], "interleaving"))
      test_interleaving();
    else if(!strcmp(argv[1], "time"))
      printf("%f\n", time_function());
    else if(!strcmp(argv[1], "sim"))
    {
      if(argc != 6)
        printf("Incorrect argument count for sim.\n");
      else
      {
        printf("%f", message_pass_rate_sim(atoi(argv[3]), atoi(argv[2]), atof(argv[4]), atoi(argv[5])));
        return 0;  //return 0 here so as not to print anything else which will prevent interpretation of piped output to float
      }
    }
    else if(!strcmp(argv[1], "interleave_with_conv"))
    {
      //test_interleave_with_conv();
    }
    else if(!strcmp(argv[1], "segmentation"))
      test_segmentation();
  }
  else
    printf("no arguments\n");

  print_memory_usage_stats();
  if(allocated() > 0)
    named_allocation_dump();
  printf("Time taken was %is\n", (int)(time(NULL) - start_time));
  return 0;

}
