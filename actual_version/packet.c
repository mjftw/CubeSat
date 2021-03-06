#include "packet.h"

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "hamming.h"
#include "memory_tracker.h"
#include "convolute.h"
#include "reed_solomon.h"
#include "interleave.h"

#define START_SEQUENCE 0x7e
uint16_t CRC_base = 0xd3e9;

//segments data without reallocation, returns num_segments * raw_data which is allocated
void segment_data(raw_data rd, unsigned int segment_size, raw_data** ret, unsigned int* num_segments)
{
  if(segment_size >= rd.length)
  {
    *num_segments = 1;
    *ret = (raw_data*)alloc_named(sizeof(raw_data), "segment_data ret");
    (*ret[0]).length = rd.length;
    (*ret[0]).data = rd.data;
    return;
  }
  *num_segments = rd.length / segment_size;
  if(*num_segments * segment_size < rd.length)
    (*num_segments)++;
  *ret = (raw_data*)alloc_named(sizeof(raw_data) * *num_segments, "segment_data ret");

  unsigned int i;
  for(i = 0; i < *num_segments; i++)
  {
    (*ret)[i].data = rd.data + segment_size * i;
    if(i == *num_segments-1)
      (*ret)[i].length = rd.length - segment_size * i;
    else
      (*ret)[i].length = segment_size;
  }
}

//allocates, constructs, and returns concatenated segments
raw_data concatenate_segments(raw_data* segments, unsigned int num_segments)
{
  unsigned int i;
  unsigned int total_length = 0;
  for(i = 0; i < num_segments; i++)
    total_length += segments[i].length;
  raw_data ret;
  ret.length = total_length;
  ret.data = (uint8_t*)alloc_named(ret.length, "concatenate_segments ret");
  for(i = 0; i < num_segments; i++)
  {
    memcpy(ret.data + i * segments[0].length, segments[i].data, segments[i].length);  //segments[0].length is always the segment size or packet size
  }
  return ret;
}

uint8_t calculate_control_field()
{
  //needs to know packet receive and send numbers
  //TODO
  return 0;
}

//placese CRC check in the last 2 bytes
uint16_t calculate_CRC(raw_data rd)
{
  rd.data[rd.length-2] = 0;
  rd.data[rd.length-1] = 0;

  raw_data tmp;
  tmp.length = rd.length;
  tmp.data = (uint8_t*)alloc_named(tmp.length, "calculate_CRC tmp.data");
  memcpy(tmp.data, rd.data, tmp.length);

  unsigned int i;
  for(i = 0; i < rd.length-2; i++)
  {
    int j;
    for(j = 7; j >= 0; j--)
    {
      if((1 << j) & rd.data[i])
      {
        rd.data[i] ^= CRC_base >> (16 - j);
        rd.data[i+1] ^= CRC_base >> (8 - j);
        rd.data[i+2] ^= CRC_base >> -j;
      }
    }
  }
  memcpy(rd.data, tmp.data, rd.length-2);  //copy back except CRC
  dealloc(tmp.data);

  uint16_t ret = rd.data[rd.length-2] << 8 | rd.data[rd.length-1];

  //printf("%x\n", ret);
  return ret;
}

raw_data packet_data(raw_data message, int rs_t, unsigned int conv_constraint)
{
  raw_data packeted;
  //1 byte flag, 1 byte control, 2 bytes CRC
  packeted.length = message.length + 4;
  packeted.data = (uint8_t*)alloc_named(packeted.length, "packet_data packeted.data");
  /*packeted.data[0] = START_SEQUENCE;
  packeted.data[1] = calculate_control_field();
  memcpy(packeted.data + 2, message.data, message.length);
  calculate_CRC(packeted);*/
  memcpy(packeted.data, message.data, message.length);

  raw_data rs_enc = rs_encode(packeted, rs_t);
  raw_data conv = convolute_constrained(rs_enc, conv_constraint);
  interleave(conv);

  dealloc(packeted.data);
  dealloc(rs_enc.data);
  return conv;
}

uint8_t unpacket_data(raw_data received, raw_data* message, int rs_t, unsigned int conv_constraint, int* bit_errors)
{
  deinterleave(received);
  raw_data deconv = deconvolute_constrained(received, NULL, conv_constraint);
  uint8_t success = rs_decode(deconv, message, rs_t, NULL);
  dealloc(deconv.data);

  //currently doens't work due to packeting issues- state etc. Also interleave/no interleaving for check must be fixed.
  /*if(bit_errors != NULL)
  {
    raw_data test = packet_data(*message, rs_t, conv_constraint);
    unsigned int i;
    for(i = 0; i < test.length; i++)
    {
      uint8_t difference = test.data[i] ^ received.data[i];
    }
  }*/
  return success;

  /*uint16_t CRC_received = (decoded.data[decoded.length-2] << 8)
    | decoded.data[decoded.length-1];
  uint16_t CRC = calculate_CRC(decoded);  //recalculate CRC independently

  message->length = decoded.length - 4;
  message->data = (uint8_t*)alloc_named(message->length, "unpacket_data ret.data");
  memcpy(message->data, decoded.data + 2, message->length);*/
}



/*
//for now data has to be 64 bytes long, might change in future
//packets can't be made and sent out of sequence because HDLC control field has
//state and counts
packet make_packet(const raw_data data)
{
  assert(data.length == 64);
  packet ret;

  ret.flag = START_SEQUENCE;
  ret.flag2 = START_SEQUENCE;
  ret.address = DEFAULT_ADDRESS;
  //HDLC control field has state and must be done in sequence.
  ret.control = calculate_HDLC_control_field();

  //CRC is computed over the address, control, and information fields
  //64 + 1 + 2 == data length + address length + control length
  ret.FCS = calculate_CRC(&(ret.address), 64 + 1 + 2);

  memcpy(ret.data, data.data, data.length);

  return ret;
}

int read_packet(raw_data* ret, const packet* p)
{
  uint16_t new_FCS = calculate_CRC(&(p->address), 64 + 1 + 2);
  if(new_FCS != p->FCS)
    return 0;
  else
  {
    ret->length = 64;  //for now
    memcpy(ret->data, p->data, 64);
    return 1;
  }
}

raw_data encode(raw_data p)
{
  raw_data ret;
  raw_data pkt;
  pkt.data = (uint8_t*)p;
  pkt.length = sizeof(packet);
  //ret = encode_block(pkt);
  ret = convolute(pkt);
  return ret;
  raw_data ret;
  ret.length = 0;
  ret.data = NULL;
  return ret;
}

raw_data decode(raw_data p, int* bit_errors)
{
  //raw_data rd = decode_block(p, bit_errors);
  raw_data rd = deconvolute(p, bit_errors);
  packet ret;
  memcpy(&ret, rd.data, rd.length);
  dealloc(rd.data);
  return ret;
  raw_data ret;
  ret.length = 0;
  ret.data = NULL;
  return ret;
}*/
