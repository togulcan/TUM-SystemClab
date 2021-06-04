#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

// to make things more readable ...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(producer) {
public:
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// initiator socket to connect producer to FIFO

	// ####################### UP TO HERE ####################### //

private:
	// event for communication between send_trigger() and producer() processes
	sc_event send_event;
	// event for informing about finished transaction
	sc_event response_event;

private:
	// member_functions, processes
	void producer_proc();
	void send_trigger();

public:
	// nb_transport_bw() - Nonblocking Transport Backward path for requests,
	//					   to be called from FIFO
	tlm_sync_enum nb_transport_bw(
			tlm_generic_payload &payload,	// ref to payload
			tlm_phase &phase,				// ref to phase
			sc_time &delay_time				// ref to delay time
	);

public:
	// constructor and call to constructor of prod2fifo_socket
	SC_CTOR(producer) : prod2fifo_socket("prod2fifo_socket") {
		// registration of nb_transport_bw function with socket
		prod2fifo_socket.register_nb_transport_bw(this, &producer::nb_transport_bw);

		// registration of processes
		SC_THREAD(producer_proc);
		SC_THREAD(send_trigger);
	}
};
