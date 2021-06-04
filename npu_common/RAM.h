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

/**
 * @file RAM.h
 * @brief Two phase AT target
 */
//=====================================================================
//  Original Authors:
//    Charles Wilson, ESLX
//    Bill Bunton, ESLX
//    Jack Donovan, ESLX
//=====================================================================
#ifndef __AT_TARGET_2_PHASE_H__ 
#define __AT_TARGET_2_PHASE_H__

#include "tlm.h"                          		        // TLM headers
#include "tlm_utils/peq_with_get.h"                   // Payload event queue FIFO
#include "tlm_utils/simple_target_socket.h"
#include "memory.h"                                   // memory storage
using namespace sc_core;
using namespace tlm;

/**
 * Module to model a static RAM.
 */
SC_MODULE(RAM) {
	SC_HAS_PROCESS(RAM);

	static const sc_time READ_RESPONSE_DELAY;
	static const sc_time WRITE_RESPONSE_DELAY;
	static const sc_time ACCEPT_DELAY;

	// *******===============================================================******* //
	// *******                         ports, sockets                        ******* //
	// *******===============================================================******* //
public:
	/// target socket
	tlm_utils::simple_target_socket<RAM> m_memory_socket;

	// *******===============================================================******* //
	// *******                    member objects & variables                 ******* //
	// *******===============================================================******* //
private:

	/// actual memory
	memory m_target_memory;

	/// response payload event queue
	tlm_utils::peq_with_get<tlm_generic_payload> m_response_PEQ;

	// *******===============================================================******* //
	// *******                      member functions, processes              ******* //
	// *******===============================================================******* //
public:
	/**
	 * Constructor for Single Phase AT target RAM.
	 *
	 * @param module_name  - Module name (required for sc_module)
	 * @param memory_size  - Memory size in bytes
	 * @param memory_width - Memory width in bytes
	 */
	RAM(sc_module_name module_name, sc_dt::uint64 memory_size, unsigned int memory_width);

	/**
	 * Implementation of call from Initiator.
	 */
	tlm_sync_enum nb_transport_fw(tlm_generic_payload &payload,
			tlm_phase &phase, sc_time &delay_time);

	/**
	 * Response Processing.
	 * This routine takes transaction responses from the m_response_PEQ.
	 * It contains the state machine to manage the communication path
	 * back to the initiator. This method is registered as an SC_METHOD
	 * with the SystemC kernel and is sensitive to m_response_PEQ.get_event()
	 */
	void begin_response_method(void);

};

#endif /* __AT_TARGET_2_PHASE_H__ */
