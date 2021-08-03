/**
 * @file DmaChannel.cpp
 * @Details Implements a AT non blocking initiator
 */

#include "reporting.h"				// Reporting convenience macros
#include "DmaChannel.h"				// Our header
#include "tlm.h"					// TLM headers
using namespace sc_core;

///  filename for reporting
static const char *filename = "DmaChannel.cpp";

//=============================================================================
//
//  Initiator thread
//
//=============================================================================
void DmaChannel::initiator_thread(void) {

	/// the address in the RAM, to/from which
	/// the DMA transfers data
	soc_address_t transaction_address;

	// utility for log messages
	std::ostringstream msg;

	while(true) {

		unsigned int n_free_slots = free_memory_addresses->num_available();
		unsigned int n_waiting_input_packets = mac_in_port->num_available();
		unsigned int n_waiting_tasks = task_queue.num_available();
		unsigned int n_free_output = mac_out_port->num_free();
		// Wait until
		// 1) there is either input from the MACs with free slot in the memory to write to or
		// 2) a command from the CPUs and the possibility to transmit packets over the output line.
		while(!(n_waiting_input_packets && n_free_slots)
				&& !(n_waiting_tasks && n_free_output)) {
			wait(
					task_queue.data_written_event()
							| mac_in_port->data_written_event()
							| free_memory_addresses->data_written_event()
							| mac_out_port->data_read_event());

			// refresh after resuming
			n_free_slots = free_memory_addresses->num_available();
			n_waiting_input_packets = mac_in_port->num_available();
			n_waiting_tasks = task_queue.num_available();
			n_free_output = mac_out_port->num_free();
		}

		//======================================================================
		// Start new transaction either based on a command or using data from
		// MAC input FIFO.
		// MAC FIFO has priority (avoid packet drops).
		//======================================================================

		if(n_free_slots > 0 && n_waiting_input_packets > 0) {
			/*
			 * Packet available in input FIFO and there is a free slot in the RAM,
			 * generate payload based on packet from MAC FIFO.
			 */

			// read a packet from the MAC FIFO
			assert(mac_in_port->nb_read(actual_packet_ptr));

			// get the address of a free memory slot
			assert(free_memory_addresses->nb_read(transaction_address));

			// the transaction will be writing data to the target
			payload.set_command(TLM_WRITE_COMMAND);

		}
		else if(n_waiting_tasks > 0) {
			/*
			 * Can't read data from the input FIFO, task is waiting to be executed.
			 * Generate payload based on command.
			 */

			// get an ip packet object with sufficiently big buffer for the data array
			// The RAM content will be transfered into this packet.
			actual_packet_ptr = ip_packet_buffer->front();
			ip_packet_buffer->pop();

			// get command from the queue, then get the address in RAM
			packet_descriptor p = task_queue.read();
			transaction_address = p.baseAddress;
			// set packet's size parameter
			actual_packet_ptr->data_size = p.size
					- sizeof(actual_packet_ptr->received)
					- sizeof(actual_packet_ptr->data_size);

			// the transaction will be reading data from the target
			payload.set_command(TLM_READ_COMMAND);

		}
		else {
			// ERROR
			REPORT_ERROR(filename, __FUNCTION__, "Error in branch structure.");
			assert(0);
		}

		// Set parameters that are common for both cases.
		// Both include transfer between a MAC FIFO, just the direction is different
		payload.set_address(transaction_address); // address was set in the specific if branches
		payload.set_data_ptr(
				reinterpret_cast<unsigned char*>(actual_packet_ptr));
		payload.set_data_length(
				sizeof(actual_packet_ptr->data_size)
						+ sizeof(actual_packet_ptr->received)
						+ actual_packet_ptr->data_size);
		payload.set_response_status(TLM_INCOMPLETE_RESPONSE);

		//==================================================================
		//	start transaction
		//==================================================================
		// Create phase and delay time objects
		tlm_phase phase = BEGIN_REQ;
		sc_time delay = SC_ZERO_TIME;

//		msg.str("");
//		msg << name() << " starting new transaction" << " for Addr:0x" << hex << setw(8)
//				<< setfill('0') << uppercase << payload.get_address() << endl << "      ";
//		msg << name() << " nb_transport_fw (GP, " << report::print(phase) << ", "
//				<< delay << ")";
//		REPORT_INFO(filename, __FUNCTION__, msg.str());

		if(do_logging & LOG_DMA)
			cout << sc_time_stamp() << " " << name() << ": trans " << &payload
					<< " sent. Addr:0x" << hex << setw(8) << setfill('0')
					<< uppercase << payload.get_address() << dec << ", phase: "
					<< phase << endl;

		//-----------------------------------------------------------------------------
		// Make the non-blocking call and decode returned status (tlm_sync_enum)
		//-----------------------------------------------------------------------------
		tlm_sync_enum return_value = initiator_socket->nb_transport_fw(payload,
				phase, delay);

		msg.str("");
		msg << name() << " " << report::print(return_value) << " (GP, "
				<< report::print(phase) << ", " << delay << ")" << endl;

		switch(return_value) {

			case TLM_COMPLETED: {
				// Early completion, not implemented in the laboratory example,
				// omitted to keep the code simpler
				REPORT_FATAL(filename, __FUNCTION__,
						"DMA: Bus completed early.");
				break;
			}			// end case TLM_COMPLETED
			case TLM_UPDATED: {

				//-----------------------------------------------------------------------------
				// Target returned UPDATED, this will be 2 phase transaction
				//    Wait the annotated delay
				//-----------------------------------------------------------------------------
				if(phase == END_REQ) {

					wait(delay); // wait the annotated delay
					if(do_logging & LOG_DMA)
						cout << sc_time_stamp() << " " << name()
								<< ": transaction waiting begin-response on backward path"
								<< endl;

//				msg << "      " << name()
//						<< " transaction waiting begin-response on backward path";
//				REPORT_INFO (filename, __FUNCTION__, msg.str() );

				}
				else {
					msg << "      " << name()
							<< " Unexpected phase for UPDATED return from target ";
					REPORT_FATAL(filename, __FUNCTION__, msg.str());
				}
				break;
			} // end case TLM_UPDATED
			case TLM_ACCEPTED: {
				// Target returned ACCEPTED -> this would be 4 phase transaction
				// Case not implemented, to keep code simple.
				REPORT_FATAL(filename, __FUNCTION__,
						"DMA: Bus returned TLM_ACCEPTED: this should not occur in 2 phase models.");
				break;
			} // end case TLM_ACCEPTED

		} // end case
		wait (transaction_finished_event);
	} // end while true
} // end initiator_thread

//=============================================================================
//
//  non-blocking transport from targets
//
//=============================================================================
tlm_sync_enum DmaChannel::nb_transport_bw(tlm_generic_payload& transaction_ref,
		tlm_phase& phase, sc_time& delay) {

	// sync status, eventually returned by the routine
	tlm_sync_enum status = TLM_COMPLETED;
	// utility for log messages
	std::ostringstream msg;

//	msg.str("");
//	msg << name() << " nb_transport_bw (GP, " << report::print(phase) << ", " << delay
//			<< ")" << "from Addr:0x" << hex << setw(8) << setfill('0') << uppercase
//			<< transaction_ref.get_address() << dec << endl;
//	REPORT_INFO(filename, __FUNCTION__, msg.str())

	if(do_logging & LOG_DMA)
		cout << sc_time_stamp() << " " << name() << ": nb_transport_bw ("
				<< &transaction_ref << ", " << report::print(phase) << ", "
				<< delay << ")" << " from Addr:0x" << hex << setw(8)
				<< setfill('0') << uppercase << transaction_ref.get_address()
				<< dec << endl;

	switch(phase) {
		case END_REQ: {
			// Target has responded with END_REQ this is a 4 phase transaction
			// Not implemented in this laboratory example.

			REPORT_FATAL(filename, __FUNCTION__,
					"DMA received END_REQ transaction on bw path");
			exit(1);
			break;
		}
		case BEGIN_RESP: {
			// Respond to begin-response
			// Expected phase in 2 phase model

			m_response_PEQ.notify(transaction_ref, delay);

			phase = END_RESP; // set appropriate return phase
			delay += m_end_rsp_delay; // wait for the response delay
			status = TLM_COMPLETED; // return status

			msg << "      " << "DMA: " << report::print(status) << " (GP, "
					<< report::print(phase) << ", " << delay << ")";
			REPORT_INFO(filename, __FUNCTION__, msg.str());
			break;

		} // end case BEGIN_RESP

		case BEGIN_REQ:
		case END_RESP: {
			// Invalid phase for backward path
			REPORT_FATAL(filename, __FUNCTION__,
					"DMA: Illegal phase on backward path.");
			exit(1);
			break;
		}

		default: {
			// Unknown phase on backward path
			REPORT_FATAL(filename, __FUNCTION__,
					"DMA: Unknown phase on the backward path.");
			exit(1);
			break;
		}
	} // end switch (phase)

	return status;
} // end backward nb transport

//=============================================================================
//
//  take commands and put them into a queue
//
//=============================================================================
tlm_sync_enum // sync status
DmaChannel::nb_transport_fw(tlm_generic_payload &gp, tlm_phase &phase,
		sc_time &delay_time) {

	if(do_logging & LOG_DMA)
		cout << sc_time_stamp() << " " << name() << ": nb_transport_fw (" << &gp
				<< ", " << report::print(phase) << ", " << delay_time << ")"
				<< " for Addr:0x" << hex << setw(8) << setfill('0') << uppercase
				<< gp.get_address() << dec << endl;

	tlm_sync_enum return_status = TLM_COMPLETED;

	switch(phase) {
		//=============================================================================
		case BEGIN_REQ: {
			// make as sure as possible that the data sent is a PacketDescriptor
			assert(gp.get_data_length() == sizeof(packet_descriptor));

			if(gp.is_write()) {
				//-----------------------------------------------------------------------------
				// CPU wants to write to DMA control register
				//-----------------------------------------------------------------------------

				delay_time += m_accept_command_delay;
				m_command_PEQ.notify(gp, delay_time);
				phase = END_REQ; // advance txn state to end request
				return_status = TLM_UPDATED; // force synchronization

			} // end WRITE
			else if(gp.is_read()) {
				//-----------------------------------------------------------------------------
				// CPU wants to read descriptor
				//-----------------------------------------------------------------------------

				delay_time += m_prepare_packet_descriptor_delay;
				m_command_PEQ.notify(gp, delay_time); // put transaction into the PEQ
				phase = END_REQ; // advance txn state to end request
				return_status = TLM_UPDATED; // force synchronization
			} // end READ
			else {
				// neither read, nor write, ERROR
				REPORT_ERROR(filename, __FUNCTION__,
						"illegal transaction command");
				assert(0);
			}
			break;
		}

			//=============================================================================
		case END_RESP: {
			// not allowed in 2 phase model
			REPORT_FATAL(filename, __FUNCTION__,
					"DMA received END_RESP on fw path.(not 2 phase modeling style)");
			exit(1);
			break;
		}

			//=============================================================================
		case END_REQ:
		case BEGIN_RESP: {
			// not allowed on  forward path
			REPORT_FATAL(filename, __FUNCTION__,
					"DMA received END_REQ or BEGIN_RESP on fw path (not allowed)");
			exit(1);
			break;
		}

			//=============================================================================
		default: {
			return_status = TLM_ACCEPTED;
			REPORT_WARNING(filename, __FUNCTION__,
					"DMA default phase encountered");

			break;
		}
	} // end of switch(phase)

	return return_status;
} //end nb_transport_fw

/**
 * Target socket response generator thread
 */
void DmaChannel::respond_to_command_thread(void) {
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
				// a write command
				// transfer request to MAC output FIFO

				// write the command into the FIFO queue
				bool written_to_q = task_queue.nb_write(*descriptor_ptr);

				// check if the command was written
				if(written_to_q == false) {
					// cannot write to FIFO -> report it
					payload_ptr->set_response_status(TLM_INCOMPLETE_RESPONSE);
					REPORT_INFO(filename, __FUNCTION__,
							"DMA engine couldn't accept DMA transfer command (FIFO full)");
				}
				else {
					payload_ptr->set_response_status(TLM_OK_RESPONSE);
					REPORT_INFO(filename, __FUNCTION__,
							"DMA accepted transfer command");
				}
			}				// end WRITE
			else if(payload_ptr->is_read()) {
				// invalid address
				payload_ptr->set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
				stringstream msg("No readable DMA control register at ");
				msg << name();
				REPORT_ERROR(filename, __FUNCTION__, msg.str());

			}				// end READ
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
					REPORT_FATAL(filename, __FUNCTION__,
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

// This thread should run when the memory responded to a transaction request, and the bw
// callback function notified the m_response_PEQ.
void DmaChannel::handle_response_thread(void) {

	tlm_generic_payload * payload_ptr;
	while(true) {
		wait(m_response_PEQ.get_event());
		REPORT_INFO(filename, __FUNCTION__, "running");
		while((payload_ptr = m_response_PEQ.get_next_transaction()) != 0) {

			// Check that the transaction had a source/destination IP packet.
			assert(actual_packet_ptr != 0);

			// if command was read, write result to MAC FIFO
			if(payload_ptr->is_read()) {
				// write to MAC out port
				bool written = mac_out_port->nb_write(actual_packet_ptr);
				if(written == false) {
					// FIFO full
					REPORT_WARNING(filename, __FUNCTION__,
							"packet dropped at the MAC out FIFO");
					n_packets_dropped_output_mac++;
				}
				else {
					// signal that address is free
					// should never block
					assert(
							this->free_memory_addresses->nb_write(
									payload_ptr->get_address()));
				}
			}
			else {
				// write corresponding descriptor into descriptor queue
				packet_descriptor pd = { (soc_address_t)payload_ptr->get_address(),
						payload_ptr->get_data_length() };
				assert(packetQueue->nb_write(pd));

				// return the pointer into the buffer
				ip_packet_buffer->push(actual_packet_ptr);
			}
			// Set pointer to zero. This shows that it does not own any object.
			// Needed in destructor for cleanup.
			actual_packet_ptr = 0;
			// notify waiting process
			transaction_finished_event.notify(SC_ZERO_TIME);
		}
	}
}

void DmaChannel::end_of_simulation() {
	// delete packet (if any) held by the actual_packet_ptr
	if(actual_packet_ptr != 0) {
		delete actual_packet_ptr;
	}
}

const sc_time DmaChannel::m_end_rsp_delay = sc_time(7, SC_NS);
const sc_time DmaChannel::m_accept_command_delay = CLK_CYCLE_BUS;
const sc_time DmaChannel::m_prepare_packet_descriptor_delay = CLK_CYCLE_BUS;
