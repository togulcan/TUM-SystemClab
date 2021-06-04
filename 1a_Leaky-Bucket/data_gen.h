#include "systemc.h"

SC_MODULE(data_gen) {
	sc_out<short unsigned int> dat;

	void generate();

	SC_CTOR(data_gen) {
		SC_THREAD(generate);
		srand(123);
	}
};
