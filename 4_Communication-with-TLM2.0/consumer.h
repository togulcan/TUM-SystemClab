#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"

// to make things easier readable ...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

SC_MODULE(consumer) {
public:
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// initiator socket to connect consumer to FIFO
	simple_target_socket<target> consum2fifo_socket;

	// ####################### UP TO HERE ####################### //

private:
	// event for communication between fetch_trigger() and consumer() processes
	sc_event fetch_event;
	// event for informing about finished transaction
	sc_event response_event;

private:
	// member_functions, processes
	void consumer_proc();
	void fetch_trigger();

public:
	// nb_transport_bw() - Nonblocking Transport Backward path for requests,
	//					   to be called from FIFO
	tlm_sync_enum nb_transport_bw(
			tlm_generic_payload &payload,	// ref to payload
			tlm_phase &phase,				// ref to phase
			sc_time &delay_time				// ref to delay time
	);

public:
	// constructor and call to constructor of consum2fifo_socket
	SC_CTOR(consumer) : consum2fifo_socket("consum2fifo_socket") {
		// register nb_transport_bw function with socket
		consum2fifo_socket.register_nb_transport_bw(this, &consumer::nb_transport_bw);

		// registration of processes
		SC_THREAD(consumer_proc);
		SC_THREAD(fetch_trigger);
	}
};
