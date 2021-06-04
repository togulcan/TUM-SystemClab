#include "systemc.h"

SC_MODULE(producer) {
public:
	// ports declaration
	sc_in<bool> clk;
	sc_out<unsigned char> d_out;
	sc_out<bool> wr_en;
	sc_in<bool> full;

	// internal signal to trigger data generation, public to enable tracing
	sc_signal<bool> send;

private:
	// process declaration
	void producer_proc();
	void send_trigger();

public:
	// constructor
	SC_CTOR(producer) {

		// register processes
		SC_THREAD(send_trigger);
		SC_THREAD(producer_proc);
		sensitive << clk.pos();

		// initialize member variables & ports
		d_out.initialize(-1);
		wr_en.initialize(false);
	}
};
