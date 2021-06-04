/*****************************************************************************

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

 *****************************************************************************/

#ifndef __SIMPLEBUSAT_H__
#define __SIMPLEBUSAT_H__

#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_get.h>

#include "globaldefs.h"
using namespace tlm;
using namespace sc_core;
using namespace tlm_utils;

/**
 * @class SimpleBusAT
 * A simple bus model for approximately timed simulations.
 * Adapted from the TLM 2.0 sample code.
 */
SC_MODULE(SimpleBusAT) {
private:
	struct ConnectionInfo {
		simple_target_socket_tagged<SimpleBusAT>* from;
		simple_initiator_socket_tagged<SimpleBusAT>* to;
		tlm::tlm_phase phase;
	};
	typedef std::map<tlm_generic_payload*, ConnectionInfo> PendingTransactions;
	typedef PendingTransactions::iterator PendingTransactionsIterator;
	typedef PendingTransactions::const_iterator PendingTransactionsConstIterator;

	// *******===============================================================******* //
	// *******                            sockets                            ******* //
	// *******===============================================================******* //
public:
	simple_target_socket_tagged<SimpleBusAT> *target_socket;
	simple_initiator_socket_tagged<SimpleBusAT> *initiator_socket;

	// *******===============================================================******* //
	// *******                  member objects, variables                    ******* //
	// *******===============================================================******* //
private:
	const unsigned int nr_of_initiators;
	const unsigned int nr_of_targets;
	const sc_time arbitration_time;
	const unsigned int m_bus_width;
	PendingTransactions mPendingTransactions;

	tlm_utils::peq_with_get<tlm_generic_payload> mPEQ;
	sc_core::sc_event mBeginRequestEvent;

	// load estimation
	/// Time spent waiting for transaction completion.
	/// Only modify its value using the MEASURE_TRANSFER_TIME macro.
	sc_time total_transfer_time;
	sc_time period_start_time;

	// *******===============================================================******* //
	// *******                   member functions, processes                 ******* //
	// *******===============================================================******* //
public:

	/**
	 * Bus constructor.
	 * @param name - the name of the module
	 * @param n_initiators - number of bus masters
	 * @param n_targets - number of bus slaves
	 * @param bus_width - data bus width in bytes
	 */
	SimpleBusAT(sc_core::sc_module_name name, unsigned int n_initiators,
			unsigned int n_targets, unsigned int bus_width);
	/// destructor
	~SimpleBusAT() {
		delete[] target_socket;
		delete[] initiator_socket;
	}
	//
	// socket callback methods
	//

	tlm_sync_enum nb_transport_fw_tagged(int initiator_id,
			tlm_generic_payload& payload, tlm_phase& phase,
			sc_time& delay_time);

	tlm_sync_enum nb_transport_bw_tagged(int portId,
			tlm_generic_payload& payload, tlm_phase& phase,
			sc_time& delay_time);

	//
	// Dummy decoder:
	// - address[31-28]: portId
	// - address[27-0]: masked address
	//

	unsigned int getPortId(const sc_dt::uint64& address) {
		return (unsigned int)address >> 28;
	}

	sc_dt::uint64 getAddressOffset(unsigned int portId) {
		return portId << 28;
	}

	sc_dt::uint64 getAddressMask(unsigned int portId) {
		return 0xfffffff;
	}

	unsigned int decode(const sc_dt::uint64& address) {
		// decode address:
		// - return initiator socket id

		return getPortId(address);
	}

	//
	// AT protocol
	//

	/**
	 * Picks transactions out of the payload event queue, classifies
	 * them according to the phase and calls appropriate functions
	 * that handle requests and responses.
	 */
	void RequestThread(void);

	/**
	 * print the load of the module
	 */
	void output_load();

private:
	void addPendingTransaction(tlm_generic_payload& trans,
			simple_initiator_socket_tagged<SimpleBusAT>* to, int initiatorId,
			tlm::tlm_phase phase) {
		const ConnectionInfo info = {&target_socket[initiatorId], to, phase};
		assert(mPendingTransactions.find(&trans) == mPendingTransactions.end());
		mPendingTransactions[&trans] = info;
	}

	inline sc_time get_transport_delay(unsigned int bytes) {
		unsigned int n_cycles = bytes / m_bus_width + 1;
		return CLK_CYCLE_BUS * n_cycles;
	}

	/**
	 * Handles a transaction object, initiates appropriate new function calls.
	 *
	 * @param trans - pointer to a transaction object
	 */
	void sendToTarget(tlm_generic_payload* payload_ptr);
	void sendToInitiator(tlm_generic_payload* payload_ptr);

};

/// Wrapper macro to record the time used for the transfer.
/// usage: Put the transaction code inside the parentheses, and
/// 	the total_transfer_time member variable will be increased
///		according to the consumed time.
/// @see MEASURE_PROCESSING_TIME
#define MEASURE_TRANSFER_TIME(code)                                 \
		period_start_time = sc_time_stamp();                        \
		code                                                        \
		total_transfer_time += sc_time_stamp() - period_start_time;

#endif
