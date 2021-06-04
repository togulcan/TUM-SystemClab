/**
 * @file	MemoryManager.h
 *
 * @date	May 10, 2011
 * @author	Miklos Kirilly
 */

#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_

#include "globaldefs.h"
#include <queue>
#include <tlm.h>	// includes systemc.h
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"

#include "IpPacket.h"
#include "packet_descriptor.h"

using namespace sc_core;
using namespace tlm;

/**
 * Memory manager module.
 * Submodule of the combined DMA engine-MAC unit, which has the following responsibilities:
 *
 * Manage the slots in the memory. It holds a list of available slot addresses
 * (@ref free_memory_addresses), and the DMA channels can only write to the memory when
 * they can get a slot address from this list. CPUs must read to the socket of
 * this submodule to drop a corrupted packet.
 *
 * @see IoModule
 * @see DmaChannel
 */
SC_MODULE(MemoryManager) {
public:

	// *******===============================================================******* //
	// *******                         ports, sockets                        ******* //
	// *******===============================================================******* //

	/// interrupt which is high when there is packet in the memory not claimed by any processor
	sc_out<bool> new_packet_IT;
	/// target socket, processors can read packet descriptors through this socket
	tlm_utils::simple_target_socket<MemoryManager> target_socket;

	// *******===============================================================******* //
	// *******                  contained and associated objects             ******* //
	// *******===============================================================******* //

	/// Buffer for managing the ip packet pointers.
	/// Using the buffer queue we can avoid re-creation of objects, thus
	/// speed up the simulation.
	std::queue<IpPacket *>* ip_packet_buffer;

	/// RAM slots not yet occupied by packet
	sc_fifo<soc_address_t> free_memory_addresses;

	/// queue that holds packet descriptors that should be
	/// read by the CPUs after the packets are transfered to
	/// the memory and the processors are notified
	sc_fifo<packet_descriptor> packet_queue;

private:
	/// payload event queue
	tlm_utils::peq_with_get<tlm_generic_payload> m_command_PEQ;

	// parameters
	/// delay introduced by the "drop packet" register when written to
	sc_time m_accept_command_delay;
	/// delay when reading the packet descriptor FIFO
	sc_time m_read_packet_descriptor_delay;

	// *******===============================================================******* //
	// *******                      member functions, processes              ******* //
	// *******===============================================================******* //
public:
	/// target socket callback method
	tlm_sync_enum nb_transport_fw(tlm_generic_payload &payload, tlm_phase &phase,
			sc_time &delay_time);

private:
	/// calls the backward path of a transaction
	void respond_to_command_thread(void);
	/// method that sets and clears the interrupt line in accordance with the packet_queue
	void interrupt_port_method(void);

	/**
	 * @fn MemoryManager
	 * constructor
	 */
public:
	SC_CTOR(MemoryManager);
};

#endif /* MEMORYMANAGER_H_ */
