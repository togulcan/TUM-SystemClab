#ifndef __CPU_H__
#define __CPU_H__

#include <tlm.h>
#include <string>
#include "tlm_utils/simple_initiator_socket.h"

#include "globaldefs.h"
#include "IpPacket.h"
#include "packet_descriptor.h"
#include "RoutingTable.h"

using namespace tlm;
using namespace tlm_utils;
using namespace sc_core;

SC_MODULE(Cpu) {

	// *******========================================================******* //
	// *******                  sockets, ports                        ******* //
	// *******========================================================******* //
public:
	/// bus master socket
	simple_initiator_socket<Cpu> initiator_socket;

	/// Interrupt line that is used by the DMA when it
	/// finishes the transfer of a received packet into the RAM.
	sc_in<bool> packetReceived_interrupt;

	// *******========================================================******* //
	// *******           member objects, variables                    ******* //
	// *******========================================================******* //
private:
	/// Unique processor ID.
	/// Assigned at construction time. It is used to access an Accelerator.
	/// This member is used in Accelerator::LookupRequest::processorId.
	const unsigned int m_id;

	/// packet descriptor sent to or read from the IO module
	packet_descriptor m_packet_descriptor;

	/// transaction payload used by the CPU for making transaction, only one
	/// instance as only one transaction at a time
	tlm_generic_payload payload;

	/// event to signal when the return path returns the read data
	sc_event transactionFinished_event;

	// *******========================================================******* //
	// *******            member functions, processes                 ******* //
	// *******========================================================******* //
private:
	/**
	 * Implementation for the initiator socket backward interface. This is the
	 * only function that is overridden for the simple_initiator_socket.
	 *
	 * It is called by the interconnect when the target responds in the second
	 * phase of the transaction.
	 */
	tlm_sync_enum nb_transport_bw(				// nb_transport
			tlm_generic_payload& transaction,	// transaction
			tlm_phase& phase,					// transaction phase
			sc_time& time);						// elapsed time

	/**
	 * Main thread, this does all the processing.
	 */
	void processor_thread(void);

	/**
	 * Start a 2-phase transaction with the given arguments.
	 *
	 * @param command - TLM_READ_COMMAND or TLM_WRITE_COMMAND
	 * @param address - the address of the destination/source of the data
	 * @param data    - pointer to the data that is written or pointer to a
	 *                  buffer where the data is going to be stored
	 * @param dataSize - size of the data in bytes
	 */
	void startTransaction(tlm_command command, soc_address_t address,
			unsigned char *data, unsigned int dataSize);

	//#############################################
	// You may add additional function from here...
	//#############################################



	//#############################################
	// Until here
	//#############################################

public:

	// *******========================================================******* //
	// *******                      constructor                       ******* //
	// *******========================================================******* //
	SC_CTOR(Cpu) :
		initiator_socket("initiator_socket"),
		m_id(Cpu::instances++)
	{
		SC_THREAD(processor_thread);
		initiator_socket.register_nb_transport_bw(this, &Cpu::nb_transport_bw);
	}

private:

	/// number of instantiated Cpu objects, used to automatically set the m_id
	/// field
	static unsigned int instances;
};

#endif /* __CPU_H__ */
