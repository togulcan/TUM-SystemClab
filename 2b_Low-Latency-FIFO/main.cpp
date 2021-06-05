#include "consumer.h"
#include "producer.h"
#include "fifo_1b.h"

int sc_main(int argc, char *argv[]) {
	// the following instruction generates a clock signal with clock named
	// "clock" with a period of 100 ns, a duty cycle of 50%, and a falling edge
	// after 50 ns
	sc_clock clk("clock", 100, SC_NS, 0.5, 50, SC_NS, false);

	// fill in the required commands to instantiate and connect producer, fifo,
	// and consumer
	sc_signal<bool> wr_en;
	sc_signal<bool> rd_en;
	sc_signal<unsigned char> d_in;
	sc_signal<unsigned char> d_out;
	sc_signal<bool> full;
	sc_signal<bool> valid;

	producer prod("producer");
	consumer cons("consumer");
	fifo_1b fifo("fifo", 5);

	// register signals
	prod.clk(clk);
	cons.clk(clk);
	fifo.clk(clk);
	prod.d_out(d_in);
	prod.wr_en(wr_en);
	prod.full(full);
	fifo.d_in(d_in);
	fifo.wr_en(wr_en);
	fifo.full(full);
	fifo.valid(valid);
	fifo.d_out(d_out);
	fifo.rd_en(rd_en);
	cons.d_in(d_out);
	cons.rd_en(rd_en);
	cons.valid(valid);

	// fill in code to generate traces that can be used to observe the
	// functionality of the model with the waveform viewer gtkwave
	sc_trace_file *tf = sc_create_vcd_trace_file("traces");
	sc_trace(tf, clk, "clock");
	sc_trace(tf, d_in, "d_in");
	sc_trace(tf, d_out, "d_out");
	sc_trace(tf, wr_en, "wr_en");
	sc_trace(tf, rd_en, "rd_en");
	sc_trace(tf, full, "full");
	sc_trace(tf, valid, "valid");

	sc_time sim_dur = sc_time(5000, SC_NS);
	if(argc != 2) {
		cout << "Default simulation time = " << sim_dur << endl;
	}
	else {
		sim_dur = sc_time(atoi(argv[1]), SC_NS);
		cout << "Simulation time = " << sim_dur << endl;
	}

	// start simulation
	sc_start(sim_dur);
	sc_close_vcd_trace_file(tf);

	return 0;
}
