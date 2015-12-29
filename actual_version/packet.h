#ifndef PACKET_H
#define PACKET_H

//This file is a header for packets and their construction

//The cc1125 has a 128 byte FIFO so 128 bytes encoded packets are sensible.
//Using Hamming(4,7) code, 73 bytes of data can be transmitted.

//Using 8 bit flag, address, 16 bit control and FCS, this leaves 64 bytes for message.

#include "datatypes.h"

//makes packet and calculates necessary values for AX25 etc
//packet make_packet(const raw_data data);
//takes a packet, and makes the data, returns true if successful or false if not.
//Reasons for returning false could be incorrect CRC etc
//if 0 is returned, raw_data returned is invalid
//int read_packet(raw_data* ret, raw_data p);  //raw data is output, packet is input

//encode and decode with Hamming(4,7)
//raw_data encode(const packet* p);
//packet decode(raw_data p, int* bit_errors);

raw_data packet_data(raw_data message, int rs_t);

//returns 1 if CRC matches
uint8_t unpacket_data(raw_data received, raw_data* ret, int rs_t);

#endif  //PACKET_H
