#include "systemc.h"

SC_MODULE(bcd_decoder) {
	sc_in<unsigned short int> val;
	sc_out<char> hi, lo;

	void decode();

	SC_CTOR(bcd_decoder) {
		SC_METHOD(decode);
		sensitive << val;
	}
};
