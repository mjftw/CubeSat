// ECC beginning.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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

	std::cout << z[0] << z[1] << z[2] << std::endl;

	bool d[4];
	d[0] = t[2];
	d[1] = t[4];
	d[2] = t[5];
	d[3] = t[6];

	int syndrome = (z[0] << 2) | (z[1] << 1) | (z[2]);
	if (syndrome == 0)
		return input;
	else
	{
		std::cout << 7 - syndrome << std::endl;
		return insert_error(input, 7 - syndrome);
	}
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


int main()
{
	/*std::fstream f;
	f.open("functions.txt", std::ios::out);
	f << make_lookup_table(hamming47, "hamming47_lookup", 16);
	f << make_lookup_table(check_byte, "check_hamming47_lookup", 128);
	f.close();
	return 0;*/


	for (unsigned int i = 0; i < 16; i++)
	{
		for (unsigned int j = 0; j < 7; j++)
		{
			char test = hamming47_lookup(i);
			char compare = test;
			print_byte(test);
			test = insert_error(test, j);
			print_byte(test);
			test = check_hamming47_lookup(test);
			print_byte(test);
			if (compare != test)
			{
				std::cout << "FAIL" << std::endl;
				return 0;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	return 0;
}

