#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdint.h>

typedef struct
{
  uint8_t flag;      //"magic" flag
  uint8_t address;   //not needed for us, but complies with AX25
  uint16_t control;  //frame numbers etc
  uint8_t data[64];  //actual frame data
  uint16_t FCS;      //frame checking sequence (CRC)
  uint8_t flag2;     //same as flag
} packet;

typedef struct
{
  uint8_t* data;
  int length;
} raw_data;

typedef raw_data encoded_packet;

#endif  //DATATYPES_H
