#include "systemc.h"

SC_MODULE(fifo_1b) {
private:
	// process declaration
	void read_write_fifo();

	// member variables
	const unsigned int fifo_size;
	unsigned char *fifo_data;
	sc_signal<unsigned int> rd_ptr;
	sc_signal<unsigned int> wr_ptr;

public:
	// ports declaration
	sc_in<bool> clk;
	// producer interface
	sc_in<unsigned char> d_in;
	sc_in<bool> wr_en;
	sc_out<bool> full;
	// consumer interface
	sc_out<unsigned char> d_out;
	sc_in<bool> rd_en;
	sc_out<bool> valid;

	// member variable, public to enable tracing
	unsigned int fill_level;

	// constructor declaration
	SC_HAS_PROCESS(fifo_1b);
	fifo_1b(sc_module_name name, unsigned int fifo_size=5);
};
