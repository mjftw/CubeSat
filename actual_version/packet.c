#include "packet.h"

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "hamming.h"
#include "memory_tracker.h"
#include "convolute.h"

#define START_SEQUENCE 0x7d
#define DEFAULT_ADDRESS 0x00

//packet is made according to HDLC frame
/*
  1 byte flag
  1 byte address
  2 bytes control (extended)
  information bytes
  2 bytes CRC
  1 byte flag
*/

uint16_t calculate_HDLC_control_field()
{
  //needs to know packet receive and send numbers
  //TODO
  return 0;
}

uint16_t calculate_CRC(const uint8_t* data, int length)
{
  //TODO
  return 0;
}

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

encoded_packet encode(const packet* p)
{
  raw_data ret;
  raw_data pkt;
  pkt.data = (uint8_t*)p;
  pkt.length = sizeof(packet);
  //ret = encode_block(pkt);
  ret = convolute(pkt);
  return ret;
}

packet decode(encoded_packet p, int* bit_errors)
{
  //raw_data rd = decode_block(p, bit_errors);
  raw_data rd = deconvolute(p, bit_errors);
  packet ret;
  memcpy(&ret, rd.data, rd.length);
  dealloc(rd.data);
  return ret;
}
