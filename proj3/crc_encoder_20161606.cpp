#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int atob(char*);
int modTwoDivision(int dataword, int generator, int dataword_size, int generator_size);
int getBinaryDigit(int num, int idx);
void printByte(FILE* opf, int codeword, int codeword_size, char* buffer, int* buffer_idx);

int main(int argc, char* argv[]) {
	
	FILE* ipf, * opf;	// input file & output file pointer
	int dataword_size, codeword_size, generator, generator_size;
	int dataword, codeword, tmp, buffer_idx = 0;
	char buffer, output_buffer = 0, numOfPadding = 0, numOfChar = 0;

	// check arguments
	if (argc != 5) {
		fprintf(stderr, "usage: ./crc_encoder input_file output_file output_generator dataword_size\n");
		return 1;
	}

	// file open
	if ((ipf = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "input file open error.\n");
		return 1;
	}


	if ((opf = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "output file open error.\n");
		return 1;
	}

	// check dataword size
	dataword_size = atoi(argv[4]);
	if (dataword_size != 4 && dataword_size != 8) {
		fprintf(stderr, "dataword size must be 4 or 8.\n");
		return 1;
	}

	// Get generator and size of the generator (in bits).
	generator = atob(argv[3]);
	tmp = generator;
	for (generator_size = 0; tmp != 0; generator_size++)
		tmp = tmp >> 1;

	// Calculate the number of padding bits and write it to the output file.
	while ((buffer = fgetc(ipf)) != EOF)
		numOfChar++;

	codeword_size = dataword_size + (generator_size - 1);
	numOfPadding = 8 - ( codeword_size * numOfChar * (8/dataword_size) % 8 );
	
	fprintf(opf, "%c", numOfPadding);
	printByte(opf, 0, numOfPadding, &output_buffer, &buffer_idx);
	fclose(ipf);

	// Read 1 byte from the input file.
	ipf = fopen(argv[1], "r");
	while ((buffer = fgetc(ipf)) != EOF) {
		codeword = 0;

		if (dataword_size == 4) {
			// xxxx0000  MSB 4 bits of dataword
			dataword = 0b1111 & (buffer >> 4);
			codeword = modTwoDivision(dataword, generator, dataword_size, generator_size);
			printByte(opf, codeword, codeword_size, &output_buffer, &buffer_idx);

			codeword = codeword << codeword_size;
			// 0000xxxx	 LSB 4 bits of dataword
			dataword = 0b1111 & buffer;
			codeword += modTwoDivision(dataword, generator, dataword_size, generator_size);
			printByte(opf, codeword, codeword_size, &output_buffer, &buffer_idx);
		}
		else if (dataword_size == 8) {
			dataword = (int)buffer;
			codeword = modTwoDivision(dataword, generator, dataword_size, generator_size);
			printByte(opf, codeword, codeword_size, &output_buffer, &buffer_idx);
		}
	}

	fclose(ipf);
	fclose(opf);

	return 0;
}

/*
	Convert the string to binary integer.
*/
int atob(char* a) {
	int n = atoi(a), b = 0, i = 0;
	while (n != 0) {
		b += (n % 10) * (int)pow(2, i);
		n /= 10;
		i++;
	}
	return b;
}

/*
	do modulo-2 division
	return codeword
*/
int modTwoDivision(int dataword, int generator, int dataword_size, int generator_size) {
	int codeword = 0, codeword_size = dataword_size+(generator_size-1);

	// pad (generator_size -1) zeros to the end of dataword.
	codeword = dataword << (generator_size - 1);

	for (int i = codeword_size ; i >= generator_size; i--) {
		if (getBinaryDigit(codeword, i) == 1) 
			codeword = codeword ^ (generator << (i - generator_size));
	}
	codeword += dataword << (generator_size - 1);

	return codeword;
}

/*
	Find (idx)th digit in binary number.

	(assume LSB = 1th bit)
*/
int getBinaryDigit(int num, int idx) {
	num = num >> (idx-1);
	return num%2;
}

void printByte(FILE* opf, int codeword, int codeword_size, char* buffer, int* buffer_idx) {
	
	// print 'codeword_size'th ~ 1th bit of codeword.
	for (int i = codeword_size; i >= 1; i--) {
		//print 'i'th bit of codeword.
		*buffer = *buffer << 1;
		*buffer += getBinaryDigit(codeword, i);
		(*buffer_idx)++;

		// if buffer is full, print
		if (*buffer_idx == 8) {
			fprintf(opf, "%c", *buffer);
			*buffer = 0;
			*buffer_idx = 0;
		}
	}
}
