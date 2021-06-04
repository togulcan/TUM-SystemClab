#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "fifo_if.h"

#define DATA_SIZE (1 << 20)
#define TEST_FIFO_1A 0
#define TEST_FIFO_1B 1
#define TEST_FIFO_2 2
#define TEST_FIFO_3 3

// to make things more readable ...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(testbench) {
public:
	// ports declaration

	// clock for fifo_1a & fifo_1b
	sc_out<bool> clk;
	// interface to fifo_1a
	sc_out<unsigned char> fifo_1a_in;
	sc_out<bool> fifo_1a_wr_en;
	sc_in<bool> fifo_1a_full;
	sc_in<unsigned char> fifo_1a_out;
	sc_out<bool> fifo_1a_rd_en;
	sc_in<bool> fifo_1a_empty;

	// interface to fifo_1b
	sc_out<unsigned char> fifo_1b_in;
	sc_out<bool> fifo_1b_wr_en;
	sc_in<bool> fifo_1b_full;
	sc_in<unsigned char> fifo_1b_out;
	sc_out<bool> fifo_1b_rd_en;
	sc_in<bool> fifo_1b_valid;

	// interface to fifo_2
	sc_port<fifo_if> fifo_2_wr_port;
	sc_port<fifo_if> fifo_2_rd_port;

	// interface to fifo_3
	simple_initiator_socket<testbench> fifo_3_wr_socket;
	simple_initiator_socket<testbench> fifo_3_rd_socket;

private:
	// member variables
	const unsigned int fifo_size;
	const unsigned int min_words;
	const unsigned int rnd_range;
	unsigned char *test_data;
	unsigned int wr_ptr;
	unsigned int rd_ptr;
	unsigned int wr_queue;
	unsigned int rd_queue;
	unsigned int fifo_test;

	// signal for fifo_1a test
	sc_signal<bool> fifo_1a_data_valid;

	// events for process communication
	sc_event write_event;
	sc_event read_event;
	sc_event start_test;
	sc_event test_finished;
	sc_event response_event_wr;
	sc_event response_event_rd;

	// member functions
	// helper function to determine number of words to write/read
	inline unsigned int rnd_num_words() {return min_words + rand() % rnd_range;};
	// reset helper function
	void reset();

	// process declaration
	void testbench_thr();
	void write_trigger();
	void read_trigger();
	void clk_gen();
	void test_fifo_1a_wr();
	void test_fifo_1a_rd();
	void test_fifo_1b_wr();
	void test_fifo_1b_rd();
	void test_fifo_2_wr();
	void test_fifo_2_rd();
	void test_fifo_3_wr();
	void test_fifo_3_rd();

public:
	// nb_transport_bw() - Nonblocking Transport Backward path for requests,
	//					   to be called from FIFO for writes
	tlm_sync_enum nb_transport_bw_wr(
			tlm_generic_payload &payload,	// ref to payload
			tlm_phase &phase,				// ref to phase
			sc_time &delay_time				// ref to delay time
	);
	// nb_transport_bw() - Nonblocking Transport Backward path for requests,
	//					   to be called from FIFO for reads
	tlm_sync_enum nb_transport_bw_rd(
			tlm_generic_payload &payload,	// ref to payload
			tlm_phase &phase,				// ref to phase
			sc_time &delay_time				// ref to delay time
	);

	// constructor
	SC_HAS_PROCESS(testbench);
	testbench(sc_module_name name, unsigned int fifo_size = 50,
			unsigned int min_words = 8, unsigned int rnd_range = 16);
};
