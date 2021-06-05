#include "bcd_decoder.h"

void bcd_decoder::decode() {
	unsigned short int input = val.read();
	if (input < 10) {
		hi.write('0');
		lo.write(char(input));
	}
	else{
		hi.write(char(int(input/10)));
		lo.write(char(input%10));
	}
}
