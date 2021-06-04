#include "systemc.h"

SC_MODULE(policer) {
	sc_in<short unsigned int> size;
	sc_out<bool> discard;

	int r_pol;
	int max;
	int counter;
	sc_time last_arrival_time;
	int dec;

	void policer_proc();

	SC_CTOR(policer) {
		SC_METHOD(policer_proc);
		sensitive << size;
		dont_initialize();

		r_pol = 30;
		max = 1500;
		counter = 0;
		last_arrival_time = SC_ZERO_TIME;
	}
};
