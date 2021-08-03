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

public:
	/// bus master socket
	simple_initiator_socket<Cpu> initiator_socket;

	/// Interrupt line that is used by the DMA when it
	/// finishes the transfer of a received packet into the RAM.
	sc_in<bool> packetReceived_interrupt;

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

	/// header of the IP packet that the processor works on (wrapper)
	IpPacket m_packet_header;

	/// Routing table instance.
	/// Not used if the system contains accelerator(s)
	RoutingTable m_rt;


	/// Time spent with computation.
	/// Only modify its value using the MEASURE_PROCESSING_TIME macro.
	sc_time total_processing_time;

	/// Time spent waiting for transaction completion.
	/// Only modify its value using the MEASURE_TRANSFER_TIME macro.
	sc_time total_transfer_time;

	// start of a measured time period
	sc_time period_start_time;

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

	/**
	 * Calculates checksum of the IP v4 packet header.
	 * @param header - Pointer to an IP packet, the checksum is calculated for
	 * 					the packets' header.
	 * @return the 16-bit checksum
	 */
	unsigned short int calculateChecksum(const IpPacket& header) const;

	/**
	 * Verifies the integrity of an IP v4 packet.
	 * -	First, the header length is checked (the standard
	 * 		requires each header to be at least 20 bytes long).
	 * -	Second, the Time To Live value is checked. Packets with
	 * 		TTL = 0 are considered corrupted.
	 * -	Third, header the checksum is controlled.
	 * @param header - Pointer to an IP packet, which is checked.
	 * @retval true - if the header is valid
	 * @retval false - if the header is corrupted
	 */
	bool verifyHeaderIntegrity(const IpPacket& header) const;

	/**
	 * perfom the next-hop lookup for the destination IP address the IP v4
	 * packet header.
	 * @param header - Reference to an IP packet, the destination IP address is
	 * 				   taken from the packets' header.
	 * @return port ID (range 0 to 3)
	 */
	unsigned int makeNHLookup(const IpPacket& header);

	/**
	 * Decrements the Time To Live value in the header.
	 * @param header - Pointer to an IP packet.
	 * @return the current TTL value
	 */
	unsigned int decrementTTL(IpPacket& header);

	/**
	 * Updates the checksum of the IP packet after processing.
	 * @note	The laboratory example only decrements the TTL value,
	 * 			if other parts are changed (e.g. due to NAT), then
	 * 			this function should be changed, so that the
	 * 			checksum is properly recomputed. ::calculateChecksum
	 * 			could be used for this purpose.
	 * @pre	::decrementTTL was called exactly once on this packet.
	 * @param header - Pointer to an IP packet.
	 */
	void updateChecksum(IpPacket& header);

	void discardDescriptor();

public:
	/**
	 * print the load of the module
	 */
	void output_load() const;
public:

	SC_CTOR(Cpu) :
		initiator_socket("initiator_socket"),
		m_id(Cpu::instances++),
		m_rt(lutConfigFile, '|')
	{
		SC_THREAD(processor_thread);
		initiator_socket.register_nb_transport_bw(this, &Cpu::nb_transport_bw);
	}

private:

	/// number of instantiated Cpu objects, used to automatically set the m_id
	/// field
	static unsigned int instances;
};

/// Wrapper macro to record the time used for the transfer.
/// usage: Put the transaction code inside the parentheses, and
///        the total_transfer_time member variable will be increased
///        according to the consumed time.
/// prerequisite: declared members sc_time period_start_time and
///               sc_time total_transfer_time
/// @see MEASURE_PROCESSING_TIME
#define MEASURE_TRANSFER_TIME(code)                                 \
		period_start_time = sc_time_stamp();                        \
		code                                                        \
		total_transfer_time += sc_time_stamp() - period_start_time;

/// Wrapper macro to record the time used for processing.
/// usage: Put the processing code inside the parentheses, and
///        the total_processing_time member variable will be increased
///        according to the consumed time.
/// prerequisite: declared members sc_time period_start_time and
///               sc_time total_processing_time
/// @see MEASURE_TRANSFER_TIME
#define MEASURE_PROCESSING_TIME(code)                               \
		period_start_time = sc_time_stamp();                        \
		code                                                        \
		total_processing_time += sc_time_stamp() - period_start_time;

#endif /* __CPU_H__ */
