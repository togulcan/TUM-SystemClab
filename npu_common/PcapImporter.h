/*
 * @file	PcapImporter.h
 *
 * @date	Mar 8, 2011
 * @author	Miklos Kirilly
 */

#ifndef PCAPIMPORTER_H_
#define PCAPIMPORTER_H_

#include <systemc>
#include <queue>
#include <map>
#include <pcap.h>
#include "IpPacket.h"
#include "globaldefs.h"

/**
 * Imports packet data and metainformation from a libpcap file. It throws away
 * the first packet, which is only used to establish a relationship between
 * simulation time and time in the captured data.
 *
 */
class PcapImporter: public sc_core::sc_module {

	// static members
private:
	/// Map of all destination addresses in packets read by importers.
	///	- key: the address
	/// - value: the number of occurences
	static std::map<unsigned int, unsigned int> address_map;

public:
	/// print the addresses to cout
	static void print_addresses();

	//
	// attributes, associations and interfaces
	//
public:
	/// Port for writing to the MAC receive FIFO
	sc_core::sc_port<sc_core::sc_fifo_out_if<IpPacket *> > out_port;

	/// packet queue
	std::queue<IpPacket *> *unused_packets_queue;

protected:
	/// the number of packets already read from the PCAP file
	unsigned int m_packets_read;

	/// handle for a PCAP file
	pcap_t *m_handle;

	/// scaling factor used for adjusting the rate of packets
	/// to the required load_thread in the simulation
	float m_time_scaling;

	/// time when the last packet was sent to the mac
	timeval m_last_packet_time;

	/// time used to transfer received packets on the Ethernet line
	sc_time m_total_transfer_time;

	/// file name
	char * m_file_name;

	//
	// member functions and processes
	//
public:
	/**
	 * Set scaling between the passing time in the simulation and the recordings.
	 * 
	 * If you want the packets from this recording, for example, to arrive at
	 * twice the speed they did in the original scenario, use setTimeScaling(2),
	 * if you want to slow down the rate by 80% use setTimeScaling(0.2).
	 * 
	 * @param ratio	- The ratio of the rate in the simulation and in the file.
	 * 				  It should be bigger than 0, for negative values absolute value is used. 
	 */
	void setTimeScaling(float ratio);

	void output_load() const;

protected:
	/// Main working thread of this module. Loads packets from the file and writes
	/// them to the FIFO port out_port;
	void load_thread();

	/**
	 * sends packet to the associated MAC
	 */
	void sendPacket(IpPacket * Packet);

	// CONSTRUCTOR and destructor
public:
	/**
	 * Constructor that opens the defined pcap file for reading. Member
	 * m_time_scaling is set to 1.0. It throws an exception if the file
	 * is not found or other errors occured while trying to open it.
	 * @param name - SystemC module name
	 * @param fileName - the path of the file that you want to open
	 * @throws Exception if the file cannot be opened
	 */
	PcapImporter(sc_core::sc_module_name name, const char * fileName);

	/// close PCAP file and print the number of read packets
	~PcapImporter();

};

#endif /* PCAPIMPORTER_H_ */
