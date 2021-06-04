#include "testbench.h"
#include "fifo_1a.h"
#include "fifo_1b.h"
#include "fifo_2.h"
#include "fifo_3.h"
#include "systemc.h"

#define TESTS_DEFINED 2

const unsigned int fifo_size[2] = {16, 2048};
const unsigned int min_words[2] = {8, 512};
const unsigned int rnd_range[2] = {16, 1024};

int sc_main(int argc, char *argv[]) {

	// test mode can be modified by command line parameter
	unsigned int test_mode = 0;
	if(argc != 2) {
		cout << "Default test mode: " << test_mode << endl;
	}
	else {
		test_mode = atoi(argv[1]);
		if(test_mode >= TESTS_DEFINED) {
			cout << "No test mode " << test_mode << " available. "
					<< "Default test mode: 0" << endl;
			test_mode = 0;
		}
		else
			cout << "Test mode: " << test_mode << endl;
	}

	// instances of the modules
	testbench testbench("testbench", fifo_size[test_mode], min_words[test_mode], rnd_range[test_mode]);
	fifo_1a fifo_1a("fifo_1a", fifo_size[test_mode]);
	fifo_1b fifo_1b("fifo_1b", fifo_size[test_mode]);
	fifo_2 fifo_2("fifo_2", fifo_size[test_mode]);
	fifo_3 fifo_3("fifo_3", fifo_size[test_mode]);

	// signals for the connection of fifo_1a & fifo_1b
	sc_signal<bool> clk;
	sc_signal<unsigned char> fifo_1a_in, fifo_1a_out, fifo_1b_in, fifo_1b_out;
	sc_signal<bool> fifo_1a_wr_en, fifo_1a_rd_en, fifo_1a_full, fifo_1a_empty;
	sc_signal<bool> fifo_1b_wr_en, fifo_1b_rd_en, fifo_1b_full, fifo_1b_valid;


	// connect instances
	testbench.clk(clk);

	// connect fifo_1a
	testbench.fifo_1a_in(fifo_1a_in);
	testbench.fifo_1a_out(fifo_1a_out);
	testbench.fifo_1a_wr_en(fifo_1a_wr_en);
	testbench.fifo_1a_rd_en(fifo_1a_rd_en);
	testbench.fifo_1a_full(fifo_1a_full);
	testbench.fifo_1a_empty(fifo_1a_empty);
	fifo_1a.clk(clk);
	fifo_1a.d_in(fifo_1a_in);
	fifo_1a.d_out(fifo_1a_out);
	fifo_1a.wr_en(fifo_1a_wr_en);
	fifo_1a.rd_en(fifo_1a_rd_en);
	fifo_1a.full(fifo_1a_full);
	fifo_1a.empty(fifo_1a_empty);

	// connect fifo_1b
	testbench.fifo_1b_in(fifo_1b_in);
	testbench.fifo_1b_out(fifo_1b_out);
	testbench.fifo_1b_wr_en(fifo_1b_wr_en);
	testbench.fifo_1b_rd_en(fifo_1b_rd_en);
	testbench.fifo_1b_full(fifo_1b_full);
	testbench.fifo_1b_valid(fifo_1b_valid);
	fifo_1b.clk(clk);
	fifo_1b.d_in(fifo_1b_in);
	fifo_1b.d_out(fifo_1b_out);
	fifo_1b.wr_en(fifo_1b_wr_en);
	fifo_1b.rd_en(fifo_1b_rd_en);
	fifo_1b.full(fifo_1b_full);
	fifo_1b.valid(fifo_1b_valid);

	// connect fifo_2
	testbench.fifo_2_wr_port(fifo_2);
	testbench.fifo_2_rd_port(fifo_2);

	// connect fifo_3
	testbench.fifo_3_wr_socket.bind(fifo_3.fifo2prod_socket);
	testbench.fifo_3_rd_socket.bind(fifo_3.fifo2consum_socket);

	// start simulation
	sc_start();

	return (0);
}
