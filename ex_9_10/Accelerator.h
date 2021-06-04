#ifndef ACCELERATOR_H_
#define ACCELERATOR_H_

#include <map>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/peq_with_get.h>

#include "globaldefs.h"
#include "RoutingTable.h"

using namespace tlm;
using namespace sc_core;
using namespace tlm_utils;

/**
 * @class Accelerator
 * HW accelerator module for packet routing.
 * The Accelerator starts operation if a
 *
 * It is a slave module, and it accepts write commands with a LookupTable as payload
 * and read commands with a single unsigned int as payload.
 */
SC_MODULE(Accelerator) {
public:
	/// target socket
	simple_target_socket<Accelerator> target_socket;

	/// Interrupt lines to the processors. Array size is defined by global variable @ref n_cpus.
	sc_out<bool> *irq;
private:

	/// Time spent for accelerator lookup.
	unsigned int ACC_IP_LOOKUP_CYCLES;

	/// buffer for requests
	sc_fifo<LookupRequest> requests;

	/// result of the lookup
	unsigned int out_port_id;

	/// routing table
	RoutingTable rt;

	peq_with_get<tlm_generic_payload> transaction_queue;

	/// Event signalled from the transaction_thread to the accelerator_thread
	/// when the result of the lookup is read by the processor.
	sc_event result_read_event;

	/// Time spent with computation.
	sc_time total_processing_time;

private:
	/// the processing thread
	void accelerator_thread();

	/// Thread that takes transactions from the PEQ, answers them and puts the payload data
	/// into the FIFO.
	void transaction_thread();

	/// nonblocking forward path callback
	tlm_sync_enum nb_transport_fw(tlm_generic_payload& payload,
			tlm_phase& phase, sc_time& delay);
public:
	/**
	 * print the load of the module
	 */
	void output_load() const;

	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// Write down the Declaration of the Constructor of the Parameterized Accelerator module

	// ####################### UP TO HERE ####################### //

	/** Destructor, frees memory allocated for irq. */
	~Accelerator() {delete[] irq;}
};

#endif /* ACCELERATOR_H_ */
