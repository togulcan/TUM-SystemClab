#include "stimulus.h"
#include "counter.h"
#include "bcd_decoder.h"

int sc_main(int argc, char *argv[]) {

	sc_signal<bool> clock, reset_n;
	sc_signal<unsigned short int> count_val;
	sc_signal<char> v_hi, v_lo;

	stimulus inst1("stimulus");
	counter inst2("counter");
	bcd_decoder inst3("decoder");

	// register signals
	inst1.clk(clock);
	inst1.rst_n(reset_n);
	inst2.clk(clock);
	inst2.rst_n(reset_n);
	inst2.cnt(count_val);
	inst3.val(count_val);

	// register trace variables
	sc_trace_file *tf = sc_create_vcd_trace_file("traces");
	sc_trace(tf, clock, "clock");
	sc_trace(tf, reset_n, "reset");
	sc_trace(tf, count_val, "cnt_val");

	int n_cycles;
	if(argc != 2) {
		cout << "default n_cycles = 2000" << endl;
		n_cycles = 2000;
	}
	else {
		n_cycles = atoi(argv[1]);
		cout << "n_cycles = " << n_cycles << endl;
	}

	sc_start(n_cycles, SC_NS);

	sc_close_vcd_trace_file(tf);

	return 0;
}
