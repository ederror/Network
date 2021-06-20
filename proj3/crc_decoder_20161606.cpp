#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int atob(char* a);
int getBinaryDigit(char num, int idx);
int readCodeword(FILE* ipf, int codeword_size, char* input_buffer, int* buffer_idx);
int modTwoDivision(int codeword, int generator, int codeword_size, int generator_size);
void printB(char c);
int main(int argc, char* argv[]) {

	FILE* ipf, * opf, *rpf;	// input file & output file pointer
	int generator_size = 0, dataword_size, codeword_size, numOfPadding;
	int codeword, generator, buffer_idx = 0;
	int dataword, tmp, num_cw = 0 , num_cw_with_error = 0;
	char input_buffer;

	if (argc != 6) {
		fprintf(stderr, "usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
		return 1;
	}

	if ((ipf = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "input file open error.\n");
		return 1;
	}

	
	if ((opf = fopen(argv[2], "w")) == NULL) {
		fprintf(stderr, "output file open error.\n");
		return 1;
	}
	
	if ((rpf = fopen(argv[3], "w")) == NULL) {
		fprintf(stderr, "result file open error.\n");
		return 1;
	}

	// check dataword size
	dataword_size = atoi(argv[5]);
	if (dataword_size != 4 && dataword_size != 8) {
		fprintf(stderr, "dataword size must be 4 or 8.\n");
		return 1;
	}

	numOfPadding = fgetc(ipf);

	// Get generator and size of the generator (in bits).
	generator = atob(argv[4]);

	for (tmp = generator, generator_size = 0; tmp != 0; generator_size++)
		tmp = tmp >> 1;
	codeword_size = dataword_size + (generator_size - 1);

	// Get rid of padding
	readCodeword(ipf, numOfPadding, &input_buffer, &buffer_idx);

	// Read codeword and check if error exists. 
	while (1) {
		codeword = readCodeword(ipf, codeword_size, &input_buffer, &buffer_idx);
		
		if (feof(ipf))
			break;

		num_cw++;

		// Check error
		if (modTwoDivision(codeword, generator, codeword_size, generator_size))
			num_cw_with_error++;

		// Write dataword to the outputfile(datastream.tx)
		if (dataword_size == 4) {
			if (num_cw % 2 == 1) {
				dataword = codeword >> (generator_size - 1);
			}
			else {
				dataword = dataword << dataword_size;
				dataword += codeword >> (generator_size - 1);
				fprintf(opf, "%c", dataword);
			}
		}
		else {	 // dataword_size == 8
			dataword = codeword >> (generator_size - 1);
			fprintf(opf, "%c", dataword);
		}
	}

	// Write the result to result.txt
	fprintf(rpf, "%d %d", num_cw, num_cw_with_error);

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
	Find (idx)th digit in binary number.

	(assume LSB = 1th bit)
*/
int getBinaryDigit(char num, int idx) {
	num = num >> (idx - 1);
	return num & 0b00000001;
}

/*
	Read one codeword.	
	return codeword
*/
int readCodeword(FILE* ipf, int codeword_size, char* input_buffer, int* buffer_idx) {
	int codeword = 0;
	for (int i = 0; i < codeword_size; i++) {
		if (*buffer_idx == 0) {
			*input_buffer = fgetc(ipf);
			*buffer_idx = 8;
		}

		codeword = codeword << 1;
		codeword += getBinaryDigit(*input_buffer, *buffer_idx);
		(*buffer_idx)--;
	}

	return codeword;
}

/*
	do modulo-2 division
	error		-> return 1
	no error	-> return 0
*/
int modTwoDivision(int codeword, int generator, int codeword_size, int generator_size) {

	for (int i = codeword_size; i >= generator_size; i--) {
		if (getBinaryDigit(codeword, i) == 1)
			codeword = codeword ^ (generator << (i - generator_size));
	}

	if (codeword == 0)
		return 0;
	else
		return 1;
}
