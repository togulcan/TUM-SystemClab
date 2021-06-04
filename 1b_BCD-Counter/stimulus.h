#include "systemc.h"

SC_MODULE(stimulus) {
	sc_out<bool> clk, rst_n;

	void c_gen();
	void r_gen();

	SC_CTOR(stimulus) {
		SC_THREAD(c_gen);
		SC_THREAD(r_gen);
	}
};
