/**********************************************************************
 The following code is derived, directly or indirectly, from the SystemC
 source code Copyright (c) 1996-2008 by all Contributors.
 All Rights reserved.
 
 The contents of this file are subject to the restrictions and limitations
 set forth in the SystemC Open Source License Version 3.0 (the "License");
 You may not use this file except in compliance with such restrictions and
 limitations. You may obtain instructions on how to receive a copy of the
 License at http://www.systemc.org/. Software distributed by Contributors
 under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
 ANY KIND, either express or implied. See the License for the specific
 language governing rights and limitations under the License.
 *********************************************************************/

//=====================================================================
/// @file RAM.cpp
//
/// @brief Implements single phase AT target
//
//=====================================================================
//  Original Authors:
//    Charles Wilson, ESLX
//    Bill Bunton, ESLX
//    Jack Donovan, ESLX
//=====================================================================
#include "RAM.h"                        // our header
#include "reporting.h"                                // reporting macros
#include "globaldefs.h"
using namespace std;
using namespace sc_core;
using namespace tlm;

static const char *filename = "RAM.cpp"; ///< filename for reporting
///Constructor
RAM::RAM(sc_module_name module_name // module name
		, sc_dt::uint64 memory_size // memory size (bytes)
		, unsigned int memory_width // memory width (bytes)
		) :
		sc_module(module_name) /// init module name
				, m_memory_socket("memory_socket") /// init socket name
				, m_target_memory /// init target's memory
		("target_memory", READ_RESPONSE_DELAY // delay for reads
				, WRITE_RESPONSE_DELAY // delay for writes
				, memory_size // memory size (bytes)
				, memory_width // memory width (bytes)
				), m_response_PEQ("response_PEQ")
{

	// register nonblocking function
	m_memory_socket.register_nb_transport_fw(this, &RAM::nb_transport_fw);

	// Register begin_reponse as an SC_METHOD
	// Used to implement force synchronization multiple timing points
	SC_METHOD (begin_response_method);
	sensitive << m_response_PEQ.get_event();
	// don't execute at the beginning, only when the event really happens
	dont_initialize();
}

//=============================================================================
// nb_transport_fw implementation calls from initiators 
//
//=============================================================================
tlm_sync_enum // synchronization state
RAM::nb_transport_fw // non-blocking transport call through Bus
(tlm_generic_payload &payload // generic payoad pointer
		, tlm_phase &phase // transaction phase
		, sc_time &delay_time) // time it should take for transport
		{
	std::ostringstream msg; // log message
	tlm_sync_enum return_status = TLM_COMPLETED;

	//-----------------------------------------------------------------------------
	// decode phase argument
	//-----------------------------------------------------------------------------
	switch(phase) {
		//=============================================================================
		case BEGIN_REQ: {

			if(do_logging & LOG_MEM)
				cout << sc_time_stamp() << " " << name() << ": trans "
						<< &payload << " received, phase "
						<< report::print(phase) << ", delay " << delay_time
						<< std::endl;

			//-----------------------------------------------------------------------------
			// Force synchronization multiple timing points by returning TLM_ACCEPTED
			// use a payload event queue to schedule BEGIN_RESP timing point
			//-----------------------------------------------------------------------------
			m_target_memory.get_delay(payload, delay_time); // get memory operation delay

			delay_time += ACCEPT_DELAY;

			m_response_PEQ.notify(payload, delay_time); // put transaction in the PEQ

			delay_time = ACCEPT_DELAY;
			phase = END_REQ; // advance txn state to end request
			return_status = TLM_UPDATED; // force synchronization

			break;
		} // end BEGIN_REQ

			//=============================================================================
		case END_RESP: {
			// only in 4 phase model
			REPORT_ERROR(filename, __FUNCTION__,
					"RAM: illegal END_RESP phase received on fw path.");
			exit(1);
		}

			//=============================================================================
		case END_REQ:
		case BEGIN_RESP: {
			msg << name()
					<< " Illegal phase received by target -- END_REQ or BEGIN_RESP";
			REPORT_FATAL(filename, __FUNCTION__, msg.str());
			return_status = TLM_ACCEPTED;
			break;
		}

			//=============================================================================
		default: {
			return_status = TLM_ACCEPTED;
			msg << name() << " default phase encountered";
			REPORT_WARNING(filename, __FUNCTION__, msg.str());
			break;
		}
	}

	return return_status;
} //end nb_transport_fw

//=============================================================================
/// begin_response method function implementation
//
// This method is statically sensitive to m_response_PEQ.get_event 
//
//=============================================================================
void RAM::begin_response_method(void) {
	std::ostringstream msg; // log message
	tlm_generic_payload *transaction_ptr; // generic payload pointer
	msg.str("");
	tlm_sync_enum status = TLM_COMPLETED;

	//-----------------------------------------------------------------------------
	//  Process all transactions scheduled for current time a return value of NULL
	//  indicates that the PEQ is empty at this time
	//-----------------------------------------------------------------------------

	while((transaction_ptr = m_response_PEQ.get_next_transaction()) != NULL) {
		msg.str("");
		msg << name() << " starting response method";

		sc_time delay = SC_ZERO_TIME;

		m_target_memory.operation(*transaction_ptr, delay); /// perform memory operation

		tlm_phase phase = BEGIN_RESP;
		delay = SC_ZERO_TIME;

		msg << "\tRAM: trans " << transaction_ptr
				<< " sent, phase BEGIN_RESP, delay SC_ZERO_TIME\n";
		if(do_logging & LOG_MEM)
			cout << sc_time_stamp() << " " << name() << ": sending trans "
					<< transaction_ptr << ", phase " << report::print(phase)
					<< endl;

		//-----------------------------------------------------------------------------
		// Call nb_transport_bw with phase BEGIN_RESP check the returned status
		//-----------------------------------------------------------------------------
		status = m_memory_socket->nb_transport_bw(*transaction_ptr, phase,
				delay);

		msg << "\t\tRAM: response: " << report::print(status) << " (GP, "
				<< report::print(phase) << ", " << delay << ")\n";

		switch(status) {

			//=============================================================================
			case TLM_COMPLETED: {
				msg << "waiting " << delay << "\n";
				REPORT_INFO(filename, __FUNCTION__, msg.str());
				next_trigger(delay); // honor the annotated delay
				return;
				break;
			}
				//=============================================================================
			default: {
				msg << name() << " invalid return status "
						<< report::print(status);
				REPORT_ERROR(filename, __FUNCTION__, msg.str());
				break;
			}
		} // end switch

	} // end while

	next_trigger(m_response_PEQ.get_event());

} //end begin_response_queue_active

const sc_time RAM::READ_RESPONSE_DELAY = sc_time(10, SC_NS);
const sc_time RAM::WRITE_RESPONSE_DELAY = sc_time(3, SC_NS);
const sc_time RAM::ACCEPT_DELAY = CLK_CYCLE_BUS;
