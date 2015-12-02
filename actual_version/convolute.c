#include "convolute.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bitstream.h"
#include "memory_tracker.h"

//currently uses (2,1,4) convolution

int peak_num_paths_considered = 0;

//inserts a bit into the encoder state
void insert_bit(uint8_t* encoder_state, uint8_t bit)
{
  *encoder_state >>= 1;
  *encoder_state |= bit << 3;
}

uint8_t encoder_lookup(uint8_t encoder_state)
{
  //v0 generated as u0 xor u1 xor u2 xor u3
  //v1 generated as u0 xor u1 xor u3
  uint8_t lookup[] = {
      0x00, 0x03, 0x02, 0x01, 0x03, 0x00, 0x01, 0x02,
      0x03, 0x00, 0x01, 0x02, 0x00, 0x03, 0x02, 0x01};

  return lookup[encoder_state];
}

raw_data convolute(raw_data rd)
{
  uint8_t encoder_state = 0;  //4 bits due to (2,1,4) convolution
  raw_data ret;
  ret.length = rd.length * 2 + 1;  //2:1 ratio (code rate = 1/2) + 8 bits to return encoder to 0 state
  ret.data = (uint8_t*)alloc_named(ret.length, "convolute ret.data");
  for(unsigned int i = 0; i < ret.length; i++)
    ret.data[i] = 0;
  unsigned int position = 0;
  unsigned int position2 = 0;
  for(unsigned int i = 0; i < rd.length * 8; i++)
  {
    uint8_t bit = get_bits_from_position(rd.data, 1, &position);
    insert_bit(&encoder_state, bit);
    insert_bits_at_position(ret.data, encoder_lookup(encoder_state), 2, &position2);
  }
  //flush encoder
  for(unsigned int i = 0; i < 4; i++)
  {
    insert_bit(&encoder_state, 0);
    insert_bits_at_position(ret.data, encoder_lookup(encoder_state), 2, &position2);
  }
  return ret;
}

//this is a single path through the code
typedef struct
{
  raw_data data;
  int metric;  //Hamming metric (bit agreement)- will be high if good, low if bad
  unsigned int position;
  uint8_t encoder_state;
} viterbi_path;

//path length is in bytes.
viterbi_path* initial_viterbi_path(unsigned int path_length)
{
  viterbi_path* ret = (viterbi_path*)alloc_named(sizeof(viterbi_path), "initial_viterbi_path ret");
  ret->encoder_state = 0;  //always starts at 0
  ret->data.length = path_length;
  ret->data.data = (uint8_t*)alloc_named(path_length, "initial_viterbi_path ret.data.data");  //decoded data
  for(unsigned int i = 0; i < ret->data.length; i++)
    ret->data.data[i] = 0;
  ret->metric = 0;    //no agreement yet
  ret->position = 0;  //bit position in decoded data
  return ret;
}

void destroy_viterbi_path(viterbi_path* vp)
{
  dealloc(vp->data.data);
  dealloc(vp);
}

//shifts in bit into the encoder state, and calculates the new position, metric etc
void shift_in_bit_viterbi_path(viterbi_path* vp, uint8_t bit, uint8_t coded_bits)
{
  insert_bit(&(vp->encoder_state), bit);
  uint8_t different_bits = coded_bits ^ encoder_lookup(vp->encoder_state);  //bitwise xor
  if(different_bits == 0x0)  //total agreement
    vp->metric += 2;
  else if(different_bits == 0x1 || different_bits == 0x2)
    vp->metric += 1;
  else if(different_bits == 0x3)
    vp->metric += 0;
  //insert decoded bit and increment position
  //printf("new_metric = %i\n", vp->metric);
  insert_bits_at_position(vp->data.data, bit, 1, &(vp->position));
}

//allows up to 32 paths to be considered at once- enough to allow converging paths
#define NUM_PATHS_POSSIBLE 32

//will allocate new_vp, and advance both by 1 by shifting in 0 or 1 into stream
//will also update metric and position etc
//coded bits are 2 bits taken from the encoded bit stream, used to calculate metrics etc
void split_viterbi_path(viterbi_path* existing_vp, viterbi_path** new_vp, uint8_t coded_bits)
{
  assert(is_valid_pointer(existing_vp));
  (*new_vp) = (viterbi_path*)alloc_named(sizeof(viterbi_path), "split_viterbi_path new_vp");
  (*new_vp)->encoder_state = existing_vp->encoder_state;
  (*new_vp)->data.length = existing_vp->data.length;
  (*new_vp)->data.data = (uint8_t*)alloc_named((*new_vp)->data.length, "split_viterbi_path (*new_vp)->data.data");
  memcpy((*new_vp)->data.data, existing_vp->data.data, existing_vp->data.length);
  (*new_vp)->metric = existing_vp->metric;
  (*new_vp)->position = existing_vp->position;

  shift_in_bit_viterbi_path(existing_vp, 0, coded_bits);
  shift_in_bit_viterbi_path(*new_vp, 1, coded_bits);
}

//num_paths_being_considered is the number of paths being considered currently
//the first num_paths_being_considered in paths must be valid
void trim_viterbi_paths(viterbi_path** paths, int* num_paths_being_considered)
{
  unsigned int metrics_per_state[16];  //only 16 possible states
  uint8_t paths_to_trim[NUM_PATHS_POSSIBLE];  //1 indicates path to be deleted
  for(unsigned int i = 0; i < NUM_PATHS_POSSIBLE; i++)
    paths_to_trim[i] = 1;  //default is empty or to trim unless otherwise set
  for(unsigned int i = 0; i < 16; i++)
    metrics_per_state[i] = 0;

  //this loop sets metrics_per_state[encoder_state] to maximum of paths being considered
  for(unsigned int i = 0; i < *num_paths_being_considered; i++)
  {
    int metrics_index = paths[i]->encoder_state & 0xf;
    if(metrics_per_state[metrics_index] < paths[i]->metric)
      metrics_per_state[metrics_index] = paths[i]->metric;
  }

  //this loop trimms any paths that are less than the critical
  //printf("num_paths_being_considered = %i\n", *num_paths_being_considered);
  for(unsigned int i = 0; i < NUM_PATHS_POSSIBLE; i++)
  {
    if(paths[i] == NULL)
      continue;
    int metrics_index = paths[i]->encoder_state & 0xf;
    if(metrics_per_state[metrics_index] > paths[i]->metric)
    {
      paths_to_trim[i] = 1;  //same as default
      destroy_viterbi_path(paths[i]);
      paths[i] = NULL;
      (*num_paths_being_considered)--;
    }
    else
    {
      uint8_t trimmed = 0;
      for(unsigned int j = 0; j < i; j++)
      {
        if(paths[j] == NULL)
          continue;
        if((paths[i]->encoder_state == paths[j]->encoder_state) && (paths[i]->metric == paths[j]->metric))
        {
          paths_to_trim[i] = 1;  //same as default
          destroy_viterbi_path(paths[i]);
          paths[i] = NULL;
          (*num_paths_being_considered)--;
          trimmed = 1;
          break;
        }
      }
      //if metric + state is not already taken or if metric is highest of metrics for that encoder state
      if(!trimmed)
        paths_to_trim[i] = 0;
    }
  }

  //this part trims the actual paths, and changes the value of num_paths_being_considered
  //it also moves the existing paths to the beginning of the paths array
  unsigned int get_index = 0, put_index = 0;
  for(; get_index < NUM_PATHS_POSSIBLE; get_index++)
  {
    if(!paths_to_trim[get_index])  //valid
    {
      paths[put_index] = paths[get_index];
      if(get_index > put_index)
        paths[get_index] = NULL;
      put_index++;
    }
    //else not valid- do nothing
  }
}

raw_data deconvolute(raw_data rd, int* bit_error_count)
{
  int num_paths_being_considered = 0;
  viterbi_path* paths[NUM_PATHS_POSSIBLE];
  for(unsigned int i = 0; i < NUM_PATHS_POSSIBLE; i++)
    paths[i] = NULL;  //initialised

  paths[0] = actual_paths[0];
  num_paths_being_considered += 1;

  unsigned int position = 0;
  while(position / 8 < rd.length)
  {
    uint8_t encoded_bits = get_bits_from_position(rd.data, 2, &position);
    unsigned int original_npbc = num_paths_being_considered;
    for(unsigned int i = 0; i < original_npbc; i++)
    {
      split_viterbi_path(paths[i], &(paths[num_paths_being_considered]), encoded_bits);
      num_paths_being_considered++;
      if(num_paths_being_considered > peak_num_paths_considered)
        peak_num_paths_considered = num_paths_being_considered;
      if(num_paths_being_considered > NUM_PATHS_POSSIBLE)
      {
        printf("ERROR: num_paths_being_considered is greater than NUM_PATHS_POSSIBLE\n");
        printf("num_paths_being_considered = %i\n", num_paths_being_considered);
        for(unsigned int i = 0; i < 16; i++)
        {
          printf("encoder state %x:\t", i);
          for(unsigned int j = 0; j < NUM_PATHS_POSSIBLE; j++)
          {
            if(paths[j] && paths[j]->encoder_state == i)
              printf("%i:%i, ", j, paths[j]->metric);
          }
          printf("\n");
        }
        assert(0);
      }
    }
    trim_viterbi_paths(paths, &num_paths_being_considered);
  }

  viterbi_path* best_path = NULL;
  for(unsigned int i = 0; i < NUM_PATHS_POSSIBLE; i++)
  {
    if(best_path == NULL && paths[i] != NULL)  //first path
      best_path = paths[i];
    else if(paths[i] != NULL && paths[i]->metric > best_path->metric)  //better path
      best_path = paths[i];
  }

  //copy best path into ret
  raw_data ret;
  ret.length = best_path->data.length;
  ret.data = (uint8_t*)alloc_named(ret.length, "deconvolute ret.data");
  memcpy(ret.data, best_path->data.data, ret.length);

  //calculate how many bits were in error
  if(bit_error_count != NULL)
    *bit_error_count += rd.length * 8 - best_path->metric;

  //destroy all paths still being used
  for(unsigned int i = 0; i < num_paths_being_considered; i++)
    if(paths[i] != NULL)
      destroy_viterbi_path(paths[i]);

  return ret;
}

unsigned int get_peak_num_paths_considered()
{
  return peak_num_paths_considered;
}

void reset_peak_num_paths_considered()
{
  peak_num_paths_considered = 0;
}
