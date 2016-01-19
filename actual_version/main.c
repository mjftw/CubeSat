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

//error inserter is a function to insert errors, so it's easy to change out
/*float pass_rate(int num_tests, int num_errors, void (*error_inserter)(uint8_t*, int, int), int interleave_data)
{
  int successes = 0;
  for(unsigned int i = 0; i < num_tests; i++)
  {
    raw_data rd;
    rd.length = 64;
    rd.data = (uint8_t*)alloc_named(rd.length, "message_pass_rate rd");
    for(unsigned int i = 0; i < 64; i++)
      rd.data[i] = 0xff;
    packet p, q;

    //make packet
    p = make_packet(rd);

    //encode packet
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
}*/

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
    rd.length = 32 + 2*t;
    rd.data = (uint8_t*)alloc_named(rd.length, "message_pass_rate_sim rd");
    unsigned int i;
    for(i = 0; i < rd.length; i++)
      rd.data[i] = rand();

    raw_data ep = convolute(rd);

    interleave(ep);  //interleaves data in place

    insert_errors2(ep.data, ep.length, BER);

    deinterleave(ep);

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
  unsigned int time_length = 1;  //seconds
  while(time(NULL) == now);
  now++;
  int t = 4;

  //setup for funciton here
  raw_data rd;
  rd.length = 64;
  rd.data = (uint8_t*)alloc_named(rd.length, "time_function rd.data");

  raw_data rd2 = packet_data(rd, t);
  insert_errors2(rd2.data, rd2.length, 0.05);
  raw_data rd3;

  while(time(NULL) - time_length < now)
  {
    unpacket_data(rd2, &rd3, t);
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

    raw_data encoded_packet = packet_data(rd, t);
    insert_errors2(encoded_packet.data, encoded_packet.length, BER);
    raw_data decoded_packet;
    unpacket_data(encoded_packet, &decoded_packet, t);

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

int main(int argc, char** argv)
{
  //purpose of main function- parse arguments and run max argv[1] tests or max errors argv[2] with BER of argv[3].
  //prints the message pass rate for these tests- to be interpreted by python.
  //many tests designed to run in parallel with different arguments
  srand(time(NULL));

  time_t start_time = time(NULL);



  /*printf("%f\n", coding_gain(0, 2, 1000));
  return 0;

  printf("codings per second is %fs\n", time_function());*/
  //return 0;

  //this part tests packeting code
  /*int correct = 0, incorrect1 = 0, incorrect2 = 0;
  for(unsigned int i = 0; i < 100; i++)
  {
    raw_data rd;
    rd.length = 64;
    rd.data = (uint8_t*)alloc_named(rd.length, "main rd.data");
    for(unsigned int i = 0; i < rd.length; i++)
      rd.data[i] = rand();

    raw_data packet = packet_data(rd, 2);

    //insert errors here
    insert_errors2(packet.data, packet.length, 0.05);
    //packet.data[1] = 0xff;

    raw_data received;
    if(unpacket_data(packet, &received, 2))
    {
      if(!memcmp(received.data, rd.data, rd.length))
        correct++;
      else
        incorrect1++;
    }
    else
      if(memcmp(received.data, rd.data, rd.length))
        correct++;
      else
      {
        incorrect2++;
        //dealloc(rd.data);
        //break;
      }

    dealloc(rd.data);
  }
  printf("correct = %i, incorrect1 = %i, incorrect2 = %i\n", correct, incorrect1, incorrect2);*/



  /*if(argc != 4)
  {
    printf("ERROR: Incorrect number of arguments\n");
    return 0;
  }

  unsigned int max_num_tests = strtol(argv[1], NULL, 10);
  unsigned int max_num_errors = strtol(argv[2], NULL, 10);
  float BER = strtof(argv[3], NULL);

  assert(BER >= 0);

  int t = 0;  //PYTHON

  printf("%f", message_pass_rate(max_num_errors, max_num_tests, BER, t));
  //printf("%f", message_pass_rate(num_tests, BER));

  //print_memory_usage_stats();


  return 0;*/

  //printf("%x\n", galois_divide(0x8e, 0x2));
  //return 0;

  //printf("%x\n", galois_divide(0x6, 0x2));
  //printf("%x\n", galois_divide(0x7, 0x3));
  //printf("%x\n", galois_multiply(0x4, 0x5));
  //printf("%x\n", galois_multiply(0xf4, 0x3));
  //return 0;

  //printf("%x\n", galois_multiply(0x1a, 0xa1) ^ galois_multiply(0x13, 0x45));
  //return 0;

  /*for(unsigned int i = 0; i < 256; i++)
  {
    for(unsigned int j = 0; j < 256; j++)
    {
      if((i ^ j) == 0x3)
        printf("%x %x\n", i, j);
    }
  }
  return 0;*/

  //this part tests reed solomon coded_bits
  /*unsigned int false_positives = 0, false_negatives = 0;
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

    //printx(encoded);
    //printf("\n");
    uint8_t num_errors = rand() % (2 * t);
    //printf("%i\n", num_errors);
    for(i = 0; i < num_errors; i++)
      encoded.data[rand() % (rd.length + 2 * t)] = rand();

    //printx(encoded);
    //printf("\n");

    raw_data decoded;
    uint8_t success = rs_decode(encoded, &decoded, t, &error_count);
    //printf(success ? "rs_decode says decoding was successful\n" : "rs_decode says decoding was not successful\n");

    //printx(rd);
    //printf("\n");
    //printx(decoded);
    //printf("\n");
    if(memcmp(rd.data, decoded.data, rd.length) && success)  //different but thinks it's right
    {
      false_positives++;
      //printf("success = %i\n", success);
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
  printf("num_correct = %i\n", correct);*/
  //printf("RS was correcty %f%% of the time.\n", (float)successes / (float)tries * 100.0);

  /*printf("%x\n", galois_multiply(0xc2, 0x2) ^ galois_multiply(0xca, 0x3));
  printf("%x\n", galois_multiply(0xf0, 0x2) ^ galois_multiply(0xd6, 0x3));

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
  return 0;

  //printf("%x\n", galois_multiply(0x2a, 0x2a));
  //printf("%x\n", galois_multiply(0x4f, 0x52));

  for(unsigned int i = 0; i < mat.length; i++)
  {
    mat.data[i] = rand();
  }

  print_matrix(mat);
  printf("%x\n", determinant(mat));

  raw_data mat2 = inverse(mat, determinant(mat));

  print_matrix(mat2);

  dealloc(mat.data);
  dealloc(mat2.data);

  if(allocated() > 0)
    named_allocation_dump();*/

  //this part tests convolution
  srand(time(NULL));

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
  //return 0;



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
  /*uint8_t message[51];
  uint8_t check_message[51];
  for(unsigned int i = 0; i < 51; i++)
    message[i] = rand();
  memcpy(check_message, message, 51);

  raw_data rd;
  rd.length = 51;
  rd.data = message;

  for(unsigned int i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  interleave(rd);

  for(unsigned int i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  deinterleave(rd);

  for(unsigned int i = 0; i < 51; i++)
    printf("%x ", rd.data[i] & 0xff);
  printf("\n");

  if(!memcmp(message, check_message, 51))
    printf("success!");
  else
    printf("Failure!");
  return 0;*/

  /*srand(time(NULL));


  unsigned int num_tests = 100;
  unsigned int max_errors = 10;

  FILE* fp;
  fp = fopen("default.csv", "w");

  time_t t = time(NULL);
  fprintf(fp, "num_errors,MPR_independent,MPR_dependent,MPR_independent_interleave,MPR_dependent_interleave\n");
  for(int errors = 0; errors <= max_errors; errors++)
  {
    printf("iteration %i / %i  \r", errors, max_errors);
    fprintf(fp, "%i,%f,%f,%f,%f\n", errors,
      pass_rate(num_tests, errors, insert_errors, 0),
      pass_rate(num_tests, errors, insert_errors_dependent, 0),
      pass_rate(num_tests, errors, insert_errors, 1),
      pass_rate(num_tests, errors, insert_errors_dependent, 1));
  }
  printf("Total time taken = %is\n", time(NULL) - t);
  printf("Average time for single test (encode + errors + decode) = %f\n", (float)(time(NULL) - t) / (100.0 * (float)num_tests * 4.0));
  fclose(fp);
  print_memory_usage_stats();*/
  if(allocated() > 0)
    named_allocation_dump();
  printf("Time taken was %is\n", (int)(time(NULL) - start_time));
  return 0;

}
