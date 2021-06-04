#include "Accelerator.h"
#include "globaldefs.h"
#include "reporting.h"

using namespace sc_core;
using namespace std;

// ############# COMPLETE THE FOLLOWING SECTION ############# //
// complete constructor parameters
Accelerator::Accelerator() :
	// enter the required initializationes here

// ####################### UP TO HERE ####################### //

		// further initializations - leave them as they are
		// Initialize requests depth and call other constructors
		requests(9),
		rt(lutConfigFile, '|'),
		transaction_queue("transaction_queue")
{

	/// provide an interrupt line per CPU
	irq = new sc_out<bool>[n_cpus];

	/// register nonblocking callback with the target socket
	target_socket.register_nb_transport_fw(this,&Accelerator::nb_transport_fw);

	/// register threads
	SC_THREAD(accelerator_thread);
	SC_THREAD(transaction_thread);
}

void Accelerator::accelerator_thread() {
	LookupRequest req;
	sc_time processing_start_time;

	// initially clear all irq lines
	for(unsigned i = 0; i < n_cpus; i++) {
		irq[i].write(false);
	}

	while(true) {
		// Get next request.
		// Blocking call waits if none is present.
		req = requests.read();

		// processing starts, log time
		processing_start_time = sc_time_stamp();

		// do lookup
		out_port_id = rt.getNextHop(req.destAddress);
		wait(ACC_IP_LOOKUP_CYCLES * CLK_CYCLE_ACC);

		// set interrupt line
		irq[req.processorId].write(true);

		// processing finished, increase total_processing_time
		total_processing_time += (sc_time_stamp() - processing_start_time);

		// wait until the processor reads the result
		wait (result_read_event);

		if(do_logging & LOG_ACC)
			cout << sc_time_stamp() << " " << name() << " result was read."
					<< endl;

		// clear interrupt line
		irq[req.processorId].write(false);

	}
}

void Accelerator::transaction_thread() {
	// pointer to the payload from the PEQ
	tlm_generic_payload* payload_ptr;

	while(true) {
		// wait until there's transaction received
		wait(transaction_queue.get_event());

		// read all transactions until the queue is empty
		while((payload_ptr = transaction_queue.get_next_transaction()) != 0) {

			if(payload_ptr->is_write()) {

				// assert that the size of payload data is correct
				assert(payload_ptr->get_data_length() == sizeof(LookupRequest));

				// nonblocking write checks if the buffer is full
				bool write_success = requests.nb_write(
						*(LookupRequest*)payload_ptr->get_data_ptr());

				// set response status accordingly
				write_success ? payload_ptr->set_response_status(TLM_OK_RESPONSE) // there's enough buffer left
						: payload_ptr->set_response_status(TLM_INCOMPLETE_RESPONSE); // buffer full

			}
			else if(payload_ptr->is_read()) {
				// assert that the size of payload data is correct
				assert(payload_ptr->get_data_length() == sizeof(unsigned int));

				// copy result to payload data
				*(unsigned int*)payload_ptr->get_data_ptr() = out_port_id;

				// set response status
				payload_ptr->set_response_status(TLM_OK_RESPONSE);

				// Signal to the accelerator_thread with no delay.
				result_read_event.notify(SC_ZERO_TIME);
			}

			// call backward path to begin response
			tlm_phase phase = BEGIN_RESP;
			sc_time delay = SC_ZERO_TIME;
			tlm_sync_enum sync; // return value of the bw call
			sync = target_socket->nb_transport_bw(*payload_ptr, phase, delay);

			// assert transaction completed
			assert((sync == TLM_COMPLETED) && (phase == END_RESP));

			// wait for the annotated time
			wait(delay);
		}
	}
}

tlm_sync_enum Accelerator::nb_transport_fw(tlm_generic_payload& payload,
		tlm_phase& phase, sc_time& delay)
{

	if(do_logging & LOG_ACC)
		cout << sc_time_stamp() << " " << name() << " received request."
				<< endl;

	// update params
	if(payload.is_write())
		// data amount that has been written determines the delay
		delay += (int)((payload.get_data_length() + bus_width - 1) / bus_width)
				* CLK_CYCLE_BUS;
	else
		delay += CLK_CYCLE_BUS; // one cycle delay to acknowledge request to the bus

	transaction_queue.notify(payload, delay);

	phase = END_REQ; // end of request phase

	if(do_logging & LOG_ACC)
		cout << "\t" << name() << ": trans " << &payload << "received, phase "
				<< report::print(phase)
				<< (payload.is_write() ? " Read" : " Write") << ", delay "
				<< delay << std::endl;

	return TLM_UPDATED; // parameters modified but transaction not yet finished

}

void Accelerator::output_load() const {
	cout << name() << " total processing time: " << total_processing_time
			<< endl;
	cout << name() << fixed << setprecision(1) << " load: processing "
			<< (total_processing_time) / (sc_time_stamp()) * 100 << "%."
			<< endl;
}

/// file name for recording
//static const char* filename = "Accelerator.cpp";
