/*
 * @file	PcapImporter.cpp
 *
 * @date	Mar 8, 2011
 * @author	Miklos Kirilly
 */

#include "EthernetLink.h"	// contains Ethernet specific constants
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <arpa/inet.h> // for formatting inet addresses
#include "PcapImporter.h"

using namespace sc_core;
using namespace std;
/***************************************************************************************************/
/**
 * utility function to compute minimum of ints
 */
inline int min(int i1, int i2) {
	return i1 > i2 ? i2 : i1;
}

/***************************************************************************************************/
//
// class implementation
//
SC_HAS_PROCESS (PcapImporter);

PcapImporter::PcapImporter(sc_module_name name, const char * fileName) :
		sc_module(name) {
	unsigned int name_length = strlen(fileName) + 1;
	m_file_name = new char[name_length];
	memcpy(m_file_name, fileName, name_length);
	// open the PCAP file
	m_handle = pcap_open_offline(m_file_name, 0);
	if(m_handle == 0) {
		std::cerr << "unable to open PCAP file" << std::endl;
		std::exit(1);
	}
	m_time_scaling = 0.001;
	m_total_transfer_time = SC_ZERO_TIME;
	SC_THREAD (load_thread);
}

PcapImporter::~PcapImporter() {
	// close the member handler if it was open
	if(m_handle != 0) {
		pcap_close (m_handle);
	}
	std::cout << name() << " received " << m_packets_read << " packets."
			<< std::endl;
	delete[] m_file_name;
}

void PcapImporter::setTimeScaling(float ratio) {
	// abs is defined for ints in the std namespace, so it cannot be used here
	m_time_scaling = ratio > 0 ? ratio : -ratio;
}

void PcapImporter::load_thread() {
	// libpcap packet header that is filled out by pcap_next()
	pcap_pkthdr pcapPacketHeader;
	// packet data
	const u_char *temp;

	// read the packets one-by-one until we run out of data or encounter a read error
	for(m_packets_read = 0;; m_packets_read++) {
		// read pcap header and packet content
		temp = pcap_next(m_handle, &pcapPacketHeader);
		if(temp == 0) {
			// error or end of file
			pcap_close (m_handle);
			m_handle = pcap_open_offline(m_file_name, 0);
			temp = pcap_next(m_handle, &pcapPacketHeader);
			// use first packet for time synchronization and immediately move
			// on to next packet
			m_last_packet_time = pcapPacketHeader.ts;
			m_packets_read--;
			continue;
		}
		// check assumptions
		assert(pcapPacketHeader.caplen < IpPacket::PACKET_MAX_SIZE);

		if(m_packets_read != 0) {
			// Skip this part for the first packet, that one is only used for time synchronization
			// between the different input MACs of the switch (that use different pcap files).

			// Wait to make the packets arrive in the MAC FIFO at the same rate as originally.
			sc_time delay_time = sc_time(
					(pcapPacketHeader.ts.tv_sec - m_last_packet_time.tv_sec)
							* 1000, SC_MS)
					+ sc_time(pcapPacketHeader.ts.tv_usec
							- m_last_packet_time.tv_usec, SC_US);

			// Transfer time of the packet on an Ethernet line
			sc_time packet_transfer_time =
//						(max(pcapPacketHeader.len*8, 512) + EthernetLink::interframe_gap_bits)
//							* EthernetLink::time_per_bit;
					((pcapPacketHeader.len * 8 > 512 ?
							pcapPacketHeader.len * 8 : 512)
							+ EthernetLink::interframe_gap_bits)
							* EthernetLink::time_per_bit;

			// wait at least as long as it takes to transfer the packet on the line
			sc_time waiting_time =
					packet_transfer_time > (m_time_scaling * delay_time) ?
							packet_transfer_time :
							(m_time_scaling * delay_time);

			// increase total transfer time
			m_total_transfer_time += packet_transfer_time;

			wait(waiting_time);

			// put data into the Packet class
			IpPacket *p;
			if(unused_packets_queue->empty()) {
				// create new packet
				p = new IpPacket();
			}
			else {
				// get a packet pointer from the queue
				p = unused_packets_queue->front();
				// remove it from the queue
				unused_packets_queue->pop();
			}
			// The Ethernet header is stripped, so the size is smaller than what
			// the PCAP size param tells.
			p->data_size = pcapPacketHeader.len
					- EthernetLink::ETHERNET_HEADER_LENGTH;
			p->received = sc_time_stamp();
			memcpy(p->packet_data, temp + EthernetLink::ETHERNET_HEADER_LENGTH,
					p->data_size);

			// only use IP v4 packets
			if(p->getVersion() == 4) {
				n_packets_received++;
				if(n_packets_received == MAX_PACKETS)
					sc_stop();

				// log destination address in static member
				unsigned int dest_address = p->getDestAddress();
				std::map<unsigned int, unsigned int>::iterator it =
						address_map.find(dest_address);
				if(it == address_map.end()) {
					// add address to address_map
					address_map.insert(
							std::pair<unsigned int, unsigned int>(dest_address,
									1));
				}
				else {
					// increment occurrence count
					it->second++;
				}

				// post packet into the MAC FIFO
				sendPacket(p);
			}

		}
		m_last_packet_time = pcapPacketHeader.ts;

	}
}

void PcapImporter::sendPacket(IpPacket * packet) {
	bool success = out_port->nb_write(packet);
	if(!success) {
		n_packets_dropped_input_mac++;
		// packet not sent into the system, push back to the queue
		unused_packets_queue->push(packet);
	}
}

//---------------------------------------------------------------------------------------
// static members
// instantiate address_map
std::map<unsigned int, unsigned int> PcapImporter::address_map = std::map<
		unsigned int, unsigned int>();

void PcapImporter::print_addresses() {
	std::map<unsigned int, unsigned int>::iterator it;
	std::cout << "------------------------------------------\n";
	std::cout << "destination addresses of received packets:\n";
	for(it = address_map.begin(); it != address_map.end(); it++) {
		in_addr inet_address_struct;
		inet_address_struct.s_addr = htonl(it->first);
		std::cout << '\t' << inet_ntoa(inet_address_struct) << ":\t"
				<< it->second << '\n';
	}
}

void PcapImporter::output_load() const {
	cout << name() << " total transfer time: " << m_total_transfer_time << endl;
	cout << name() << fixed << setprecision(1) << " load: transfer "
			<< (m_total_transfer_time) / (sc_time_stamp()) * 100 << "%."
			<< endl;
}
