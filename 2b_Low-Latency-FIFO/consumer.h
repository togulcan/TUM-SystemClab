#include "systemc.h"

SC_MODULE(consumer) {
public:
	// ports declaration
	sc_in<bool> clk;
	sc_in<unsigned char> d_in;
	sc_out<bool> rd_en;
	sc_in<bool> valid;

	// internal signal, public to enable tracing
	sc_signal<bool> fetch;

private:
	// process declaration
	void fetch_trigger();
	void consumer_proc();

public:
	// constructor
	SC_CTOR(consumer) {

		// register processes
		SC_THREAD(fetch_trigger);
		SC_THREAD(consumer_proc);
		sensitive << clk.pos();

		// initialization of port
		rd_en.initialize(false);
	}
};
