#ifndef PACKET_H
#define PACKET_H

//This file is a header for packets and their construction

//The cc1125 has a 128 byte FIFO so 128 bytes encoded packets are sensible.
//Using Hamming(4,7) code, 73 bytes of data can be transmitted.

//Using 8 bit flag, address, 16 bit control and FCS, this leaves 64 bytes for message.

#include "datatypes.h"

//makes packet and calculates necessary values for AX25 etc
packet make_packet(const raw_data data);
//takes a packet, and makes the data, returns true if successful or false if not.
//Reasons for returning false could be incorrect CRC etc
//if 0 is returned, raw_data returned is invalid
int read_packet(raw_data* ret, const packet* p);  //raw data is output, packet is input

//encode and decode with Hamming(4,7)
encoded_packet encode(const packet* p);
packet decode(encoded_packet p, int* bit_errors);

#endif  //PACKET_H
