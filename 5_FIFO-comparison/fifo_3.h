#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"

using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(fifo_3) {
public:
	// target sockets to connect FIFO with producer and consumer
	simple_target_socket<fifo_3> fifo2prod_socket;
	simple_target_socket<fifo_3> fifo2consum_socket;

private:
	const unsigned int fifo_size;
	unsigned char *fifo_data;
	unsigned int wr_ptr, rd_ptr;
	unsigned int fill_level;

	// we use two PEQs, one for read transactions and one for write transactions
	peq_with_get<tlm_generic_payload> r_peq;
	peq_with_get<tlm_generic_payload> w_peq;

	// the processes that perform read and write
	void read_fifo();
	void write_fifo();

public:
	// nb_transport_fw() - Nonblocking Transport Forward path for requests,
	//					   to be called from producer or consumer
	tlm_sync_enum nb_transport_fw(
			tlm_generic_payload &payload,	// ref to payload
			tlm_phase &phase,				// ref to phase
			sc_time &delay_time				// ref to delay time
	);

private:
	// declaration of helper function
	void output_fifo_status();

public:
	// constructor declaration
	SC_HAS_PROCESS(fifo_3);
	fifo_3(sc_module_name name, unsigned int fifo_size=50);
};
