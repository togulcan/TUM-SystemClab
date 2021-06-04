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

//==============================================================================
/// @file DmaChannel.h
//
/// @brief Declaration of the DMA channel class.
///
/// @details Implemented as TLM AT bus module, with both master and slave
/// 		 functionality
// 
//=============================================================================
//  Original Authors:
//    Bill Bunton, ESLX
//    Anna Keist, ESLX
//
//=============================================================================
#ifndef __SELECT_INITIATOR_H__
#define __SELECT_INITIATOR_H__

#include <tlm.h>                                   // TLM headers
#include <queue>
#include "tlm_utils/peq_with_get.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "globaldefs.h"
#include "IpPacket.h"
#include "packet_descriptor.h"

#include <iomanip>

using namespace sc_core;
using namespace tlm;

/**
 * @class DmaChannel
 * Model of a DMA channel that serves a single MAC, transferring packets between
 * it and the memory.
 */
SC_MODULE( DmaChannel) {

private:
	struct TransferCommand {
		soc_address_t from;
		unsigned int to;
		unsigned int size;
	};
public:
	//==============================================================================
	// Ports, exports and Sockets
	//==============================================================================
public:
	/// FIFO write interface to access the incomming packets in the MAC inward FIFO
	sc_port<sc_fifo_in_if<IpPacket *> > mac_in_port;
	/// FIFO write interface to access the MAC outgoing FIFO
	sc_port<sc_fifo_out_if<IpPacket *> > mac_out_port;
	tlm_utils::simple_initiator_socket<DmaChannel> initiator_socket;
	tlm_utils::simple_target_socket<DmaChannel> target_socket;

	/// Buffer for managing the ip packet pointers.
	/// Using the buffer queue we can avoid re-creation of objects, thus
	/// speed up the simulation.
	std::queue<IpPacket *>* ip_packet_buffer;

	/// RAM slots not yet occupied by packet.
	/// @note Declared public so that it can be set directly.
	sc_fifo<soc_address_t> *free_memory_addresses;

	/// Queue that holds packet descriptors. Supposed to be
	/// read by the CPUs after the packets are transfered to
	/// the memory and the processors are notified.
	/// @note Declared public so that it can be set directly.
	sc_fifo<packet_descriptor> *packetQueue;

	SC_CTOR(DmaChannel):
	initiator_socket("initiator_socket") // init socket name
	, target_socket("target_socket"),
	m_response_PEQ("response_PEQ"), m_command_PEQ("command_PEQ") {

		// register callback with initiator socket
		initiator_socket.register_nb_transport_bw(this, &DmaChannel::nb_transport_bw);
		// register callback with target socket
		target_socket.register_nb_transport_fw(this, &DmaChannel::nb_transport_fw);

		// register thread processes
		SC_THREAD(initiator_thread);
		SC_THREAD(respond_to_command_thread);
		SC_THREAD(handle_response_thread);
	}
	void end_of_simulation();

private:
	/// initiator thread, starts DMA transfers
	void initiator_thread(void);

	/// this thread sends the response to access transactions from a CPU
	void respond_to_command_thread(void);

	/// thread that handles responses from the memory in transactions started from
	/// initiator_thread ()
	void handle_response_thread(void);

	//=============================================================================
	///	@brief Implementation of call from targets.
	//
	///	@details
	///		This is the ultimate destination of the nb_transport_bw call from
	///		the targets after being routed trough a Bus
	//
	//=====================================================================
	tlm_sync_enum nb_transport_bw( // nb_transport
			tlm_generic_payload& transaction, // transaction
			tlm_phase& phase, // transaction phase
			sc_time& time); // elapsed time

	//==============================================================================
	// target socket callback
	//==============================================================================
	tlm_sync_enum // sync status
	nb_transport_fw(tlm_generic_payload &gp ///< generic payoad pointer
			, tlm_phase &phase ///< transaction phase
			, sc_time &delay_time ///< time taken for transport
			);

	//==============================================================================
	// Private member variables and methods
	//==============================================================================
private:

	/// payload used for DMA transactions on the system bus
	tlm_generic_payload payload;

	/// pointer to the IP packet used in the current transaction.
	/// It points either to a packet from the MAC FIFO (MAC->RAM transfer)
	/// or to one that is going to be sent (RAM->MAC transfer).
	IpPacket* actual_packet_ptr;

	/// event notified when a transaction finishes, so that the DMA can start a new one
	sc_event transaction_finished_event;

	tlm_utils::peq_with_get<tlm_generic_payload> m_response_PEQ;
	/// Event queue for scheduling "free up memory" commands
	tlm_utils::peq_with_get<tlm_generic_payload> m_command_PEQ;

	// transaction delays

	static const sc_time m_end_rsp_delay;
	static const sc_time m_accept_command_delay;
	static const sc_time m_prepare_packet_descriptor_delay;

	/// queue that holds transfer commands received from CPUs
	sc_fifo<packet_descriptor> task_queue;

}; // end of class DmaChannel

//===========================================================
/// stream operator for TransferCommand struct
//===========================================================
/*inline std::ostream& operator<<(std::ostream& o, const DmaChannel::TransferCommand& comm) {
	o << "command: " << comm.size << " bytes at address " << std::hex << comm.from
			<< std::dec << " to MAC" << comm.to;
	return o;
 }*/

#endif /* __SELECT_INITIATOR_H__ */
