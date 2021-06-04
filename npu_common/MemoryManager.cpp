/**
 * @file	MemoryManager.cpp
 *
 * @date	May 10, 2011
 * @author	Miklos Kirilly
 */

#include "MemoryManager.h"
#include <iostream>
#include "reporting.h"

static const char *filename = "MemoryManager.cpp"; ///< filename for reporting

//---------------------------------------------------------------
// constructor
//---------------------------------------------------------------
MemoryManager::MemoryManager(sc_module_name name) :
		sc_module(name), m_command_PEQ("command_PEQ") {

	// register callback
	target_socket.register_nb_transport_fw(this,
			&MemoryManager::nb_transport_fw);
	// fill free slots queue with all the addresses
	for(unsigned int i = 0; i < n_memory_slots; i++) {
		free_memory_addresses.nb_write(
				MEMORY_BASE_ADDRESS + i * IpPacket::PACKET_MAX_SIZE);
	}

	m_accept_command_delay = CLK_CYCLE_BUS;
	m_read_packet_descriptor_delay = CLK_CYCLE_BUS;

	// register processes
	SC_THREAD (respond_to_command_thread);
	SC_METHOD (interrupt_port_method);
	// IT signal is modified if packet_queue is read or written.
	sensitive << packet_queue.data_read_event()
			<< packet_queue.data_written_event();

}

//----------------------------------------------------------
// callback function
//----------------------------------------------------------
tlm_sync_enum MemoryManager::nb_transport_fw(tlm_generic_payload &payload,
		tlm_phase &phase, sc_time &delay_time) {
	tlm_sync_enum return_status = TLM_COMPLETED;

	switch(phase) {
		//=============================================================================
		case BEGIN_REQ: {
			// make as sure as possible that the data sent is a PacketDescriptor
			assert(payload.get_data_length() == sizeof(packet_descriptor));

			if(payload.is_write()) {
				//-----------------------------------------------------------------------------
				// CPU wants to write - drop packet/free slot requested
				//-----------------------------------------------------------------------------

				delay_time += m_accept_command_delay;
				m_command_PEQ.notify(payload, delay_time);
				phase = END_REQ; // advance txn state to end request
				return_status = TLM_UPDATED; // force synchronization

			} // end WRITE
			else if(payload.is_read()) {
				//-----------------------------------------------------------------------------
				// CPU wants to read descriptor
				//-----------------------------------------------------------------------------

				delay_time += m_read_packet_descriptor_delay;
				m_command_PEQ.notify(payload, delay_time); // put transaction into the PEQ
				phase = END_REQ; // advance txn state to end request
				return_status = TLM_UPDATED; // force synchronization
			} // end READ
			else { // TLM_IGNORE_COMMAND
				   // neither read, nor write, ignore
				REPORT_WARNING(filename, __FUNCTION__, "ignoring transaction");
			}
			break;
		}

			//=============================================================================
		default: { // END_REQ, BEGIN_RESP or END_RESP; UNINITIALIZED_PHASE or extended phase
			REPORT_ERROR(filename, __FUNCTION__, "Invalid phase encountered");
			exit(1);
			break;
		}
	} // end of switch(phase)

	return return_status;
}

//---------------------------------------------------------------
// thread to call backward path
//---------------------------------------------------------------
void MemoryManager::respond_to_command_thread() {
	// payload pointer, it is set when reading from the payload event queue
	tlm_generic_payload *payload_ptr;

	// a pointer to the data of the payload, casted to represent a packet descriptor
	packet_descriptor * descriptor_ptr;

	// loop until simulation terminates
	while(true) {
		// wait until a new command was received
		wait(m_command_PEQ.get_event());

		// Then execute all commands that are waiting.
		// This should only be a single transaction, since this processing
		// takes less time than transferring a descriptor on the bus,
		// just to make sure.
		while((payload_ptr = m_command_PEQ.get_next_transaction()) != 0) {

			descriptor_ptr =
					reinterpret_cast<packet_descriptor*>(payload_ptr->get_data_ptr());

			if(payload_ptr->is_write()) {
				// a write command, drop a packet and free RAM slot
				free_memory_addresses.nb_write(descriptor_ptr->baseAddress);

				payload_ptr->set_response_status(TLM_OK_RESPONSE);
				REPORT_INFO(filename, __FUNCTION__,
						"DMA accepted drop command");
				n_packets_dropped_header++;
			}		// end WRITE
			else if(payload_ptr->is_read()) {
				// CPU wants descriptor of new packet, containing base address in RAM and size
				if(packet_queue.nb_read(*descriptor_ptr)) {
					payload_ptr->set_response_status(TLM_OK_RESPONSE);
					REPORT_INFO(filename, __FUNCTION__,
							"DMA supplied packet descriptor.");
				}
				else {
					// no packet found, probably because another processor was quicker
					payload_ptr->set_response_status(
							TLM_BURST_ERROR_RESPONSE);
					REPORT_INFO(filename, __FUNCTION__,
							"DMA could not supply packet descriptor.");
				}
			}		// end READ
			else {
				payload_ptr->set_response_status(TLM_COMMAND_ERROR_RESPONSE);
				REPORT_ERROR(filename, __FUNCTION__,
						"DMA received payload with neither read, nor write command.");
			}

			// call backward path
			// Create phase and delay time objects
			tlm_phase phase = BEGIN_RESP;
			sc_time delay = SC_ZERO_TIME;

			tlm_sync_enum sync = target_socket->nb_transport_bw(*payload_ptr,
					phase, delay);
			switch(sync) {
				case TLM_COMPLETED:
					// Transaction finished
					wait(delay);
					break;
				case TLM_ACCEPTED:
				case TLM_UPDATED:
					// Transaction not yet finished
					// Error, shouldn't happen in 2-phase model
					REPORT_ERROR(filename, __FUNCTION__,
							"Transaction not completed after DMA sent response. (ACCEPT or UPDATED)")
					assert(false);
					break;
				default:
					assert(0);
					exit(1);
			};

		}
	}
}

void MemoryManager::interrupt_port_method() {
	if(packet_queue.num_available() > 0) {
		new_packet_IT.write(true);
	}
	else {
		new_packet_IT.write(false);
	}
}
