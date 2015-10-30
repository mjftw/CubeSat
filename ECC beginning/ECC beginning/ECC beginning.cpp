// ECC beginning.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdlib>

char hamming47(char input)
{
	//assumes high 4 bits are already 0, masks them out
	input &= 0xf;
	char output = 0;
	bool d[4], p[3];
	d[0] = input & 0x1;
	d[1] = (input & 0x2) >> 1;
	d[2] = (input & 0x4) >> 2;
	d[3] = (input & 0x8) >> 3;
	p[0] = d[0] ^ d[1] ^ d[3];
	p[1] = d[0] ^ d[2] ^ d[3];
	p[2] = d[1] ^ d[2] ^ d[3];

	output = (p[0] << 6) | (p[1] << 5) | (d[0] << 4) | (p[2] << 3) | (d[1] << 2) | (d[2] << 1) | (d[3]);
	return output;
}

void print_byte(char input)
{
	for (unsigned int i = 0; i < 8; i++)
	{
		std::cout << ((input & 0x80) >> 7);
		input <<= 1;
	}
	std::cout << std::endl;
}

char insert_error(char input, int position)
{
	return (input ^ (1 << position));
}

char check_byte(char input)
{
	input &= 0x7f;  //mask out last bit (not needed)
	//checks a hamming47 encoded byte for errors/corrections
	bool t[7];
	for (unsigned int i = 0; i < 7; i++)
	{
		t[i] = (input >> (6 - i)) & 0x1;
	}
	bool z[3] = { 0, 0, 0 };  //syndrome
	z[2] = t[0] ^ t[2] ^ t[4] ^ t[6];
	z[1] = t[1] ^ t[2] ^ t[5] ^ t[6];
	z[0] = t[3] ^ t[4] ^ t[5] ^ t[6];

	//std::cout << z[0] << z[1] << z[2] << std::endl;

	int syndrome = (z[0] << 2) | (z[1] << 1) | (z[2]);

	char corrected;
	if (syndrome == 0)
		corrected = input;
	else
	{
		corrected = insert_error(input, 7 - syndrome);
	}

	bool d[4];
	d[0] = (corrected >> 4) & 0x1;
	d[1] = (corrected >> 2) & 0x1;
	d[2] = (corrected >> 1) & 0x1;
	d[3] = (corrected >> 0) & 0x1;

	return (d[3] << 3) | (d[2] << 2) | (d[1] << 1) | (d[0]);
}

char byte_errors(char input)
{
	input &= 0x7f;  //mask out last bit (not needed)
	//checks a hamming47 encoded byte for errors/corrections
	bool t[7];
	for (unsigned int i = 0; i < 7; i++)
	{
		t[i] = (input >> (6 - i)) & 0x1;
	}
	bool z[3] = { 0, 0, 0 };  //syndrome
	z[2] = t[0] ^ t[2] ^ t[4] ^ t[6];
	z[1] = t[1] ^ t[2] ^ t[5] ^ t[6];
	z[0] = t[3] ^ t[4] ^ t[5] ^ t[6];

	//std::cout << z[0] << z[1] << z[2] << std::endl;

	int syndrome = (z[0] << 2) | (z[1] << 1) | (z[2]);

	char corrected;
	if (syndrome == 0)
		return 0;
	else
		return 1;
}

std::string hex(int num)
{
	std::stringstream ss;
	ss << "0x" << std::hex << num;
	return ss.str();
}

std::string make_lookup_table(char (*function)(char), std::string fname, int max_input = 256, int columns = 8)
{
	std::stringstream ss;
	ss << "char " << fname << "(char input)" << std::endl;
	ss << "{" << std::endl;
	ss << "\tstatic const char lookup[] = {";
	for (unsigned int i = 0; i < max_input; i++)
	{
		if (i % columns == 0)
			ss << std::endl << "\t\t";
		ss << hex(function(i));
		if (i != max_input - 1)
			ss << ", ";
	}
	ss << "};" << std::endl << std::endl;

	ss << "\treturn lookup[input];" << std::endl << "}" << std::endl;

	return ss.str();
}

char hamming47_lookup(char input)
{
	static const char lookup[] = {
		0x0, 0x70, 0x4c, 0x3c, 0x2a, 0x5a, 0x66, 0x16,
		0x69, 0x19, 0x25, 0x55, 0x43, 0x33, 0xf, 0x7f };

	return lookup[input];
}

char check_hamming47_lookup(char input)
{
	static const char lookup[] = {
		0x0, 0x0, 0x0, 0x43, 0x0, 0x25, 0x16, 0xf,
		0x0, 0x19, 0x2a, 0xf, 0x4c, 0xf, 0xf, 0xf,
		0x0, 0x19, 0x16, 0x33, 0x16, 0x55, 0x16, 0x16,
		0x19, 0x19, 0x5a, 0x19, 0x3c, 0x19, 0x16, 0xf,
		0x0, 0x25, 0x2a, 0x33, 0x25, 0x25, 0x66, 0x25,
		0x2a, 0x69, 0x2a, 0x2a, 0x3c, 0x25, 0x2a, 0xf,
		0x70, 0x33, 0x33, 0x33, 0x3c, 0x25, 0x16, 0x33,
		0x3c, 0x19, 0x2a, 0x33, 0x3c, 0x3c, 0x3c, 0x7f,
		0x0, 0x43, 0x43, 0x43, 0x4c, 0x55, 0x66, 0x43,
		0x4c, 0x69, 0x5a, 0x43, 0x4c, 0x4c, 0x4c, 0xf,
		0x70, 0x55, 0x5a, 0x43, 0x55, 0x55, 0x16, 0x55,
		0x5a, 0x19, 0x5a, 0x5a, 0x4c, 0x55, 0x5a, 0x7f,
		0x70, 0x69, 0x66, 0x43, 0x66, 0x25, 0x66, 0x66,
		0x69, 0x69, 0x2a, 0x69, 0x4c, 0x69, 0x66, 0x7f,
		0x70, 0x70, 0x70, 0x33, 0x70, 0x55, 0x66, 0x7f,
		0x70, 0x69, 0x5a, 0x7f, 0x3c, 0x7f, 0x7f, 0x7f };

	return lookup[input];
}

void hamming47_encode_block(char* block, char* output, int length)
{
	//length is length in bytes of input. Will encode each byte to 2 bytes. Output must already be allocated.
	for (unsigned int i = 0; i < length; i++)
	{
		output[2 * i] = hamming47_lookup(block[i] >> 4);
		output[2 * i + 1] = hamming47_lookup(block[i] & 0xf);
	}
}

void hamming47_decode_block(char* block, char* output, int length)
{
	//length is length in bytes of output. Will decode each 2 bytes to 1 byte. Output must already be allocated.
	for (unsigned int i = 0; i < length; i++)
	{
		output[i] = check_byte(block[2 * i]) << 4;
		output[i] |= check_byte(block[2 * i + 1]);
	}
}

int random(int min, int max)
{
	return rand() % (max - min) + min;
}

void insert_errors(char* block, int length, int num_errors)
{
	//currently no protection against flipping the same bit twice, so should not be used to test bit error rates yet.
	for (unsigned int i = 0; i < num_errors; i++)
	{
		int error_position = random(0, length * 8);
		block[error_position / 8] = insert_error(block[error_position / 8], error_position % 8);
	}
}

float test_corruption_rate(int errors, int bytes)
{
	int corrupted = 0;
	for (unsigned int i = 0; i < 100000; i++)
	{
		bool* b = new bool[bytes];
		for (unsigned int j = 0; j < bytes; j++)
			b[j] = 0;
		for (unsigned int j = 0; j < errors; j++)
		{
			int index = random(0, bytes);
			if (b[index])
			{
				corrupted++;
				break;
			}
			else
				b[index] = 1;  //error in random byte
		}
		delete[] b;
	}
	return static_cast<float>(corrupted) / 100000.0;
}

int main()
{
	/*std::fstream csv;
	csv.open("BER_corruption_rate.csv", std::ios::out);
	if (!csv.is_open())
	{
		std::cout << "Unable to open file" << std::endl;
		return 0;
	}
	csv << "BER,";
	for (unsigned int i = 16; i <= 256; i += 16)
		csv << "length=" << i << " bytes,";
	csv << std::endl;

	for (float i = 0; i < 0.1; i += 0.001)
	{
		//i is BER
		csv << i << ",";
		for (unsigned int j = 16; j <= 256; j += 16)
		{
			int num_errors = i * j * 8.0;
			csv << test_corruption_rate(num_errors, j) << ",";
		}
		csv << std::endl;
		std::cout << i << std::endl;
	}
	csv.close();
	return 0;*/



	/*srand(time(NULL));
	char* test = "Hello world, this is a test block for Hamming encoding. If any bit errors go undetected, DATA CORRUPTED will be written to the terminal.";
	int test_length = strlen(test) + 1;  //+1 is for \0 character
	std::cout << test_length << std::endl;
	char* encoded = new char[test_length * 2];
	char* received = new char[test_length];

	float bit_error_rate = 4;  //percentage
	int bit_errors = (bit_error_rate * test_length * 8.0) / 100.0;

	hamming47_encode_block(test, encoded, test_length);

	insert_errors(encoded, 2 * test_length, 15);

	hamming47_decode_block(encoded, received, test_length);

	std::cout << received << std::endl;

	if (strcmp(received, test))
	{
		std::cout << "DATA CORRUPTED" << std::endl;
	}
	return 0;*/



	std::fstream f;
	f.open("functions.txt", std::ios::out);
	f << make_lookup_table(hamming47, "hamming47_lookup", 16);
	f << make_lookup_table(check_byte, "check_hamming47_lookup", 128);
	f << make_lookup_table(byte_errors, "byte_error", 128);
	f.close();
	return 0;

	/*
	char data[4] = { 0x11, 0x22, 0x33, 0x44 };

	short int check = *reinterpret_cast<short int*>(data + 1);
	std::cout << std::hex << check << std::endl;

	NOTE: can use this kind of method for CRC, but dependent on endianness of system etc. Will have to test on ARM core
	*/

	for (unsigned int i = 0; i < 16; i++)
	{
		for (unsigned int j = 0; j < 7; j++)
		{
			char test = hamming47_lookup(i);
			char compare = test;
			print_byte(test);
			test = insert_error(test, j);
			//print_byte(test);
			test = check_byte(test);
			print_byte(test);
			if (i != test)
			{
				std::cout << i << ", " << (int)test << std::endl;
				std::cout << "FAIL" << std::endl;
				return 0;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	return 0;
}
