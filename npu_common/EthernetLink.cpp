/**
 * @file	EthernetLink.cpp
 *
 * @date	May 10, 2011
 * @author	Miklos Kirilly
 */

#include "EthernetLink.h"

#include <iostream>			///< for logging
#include <iomanip>			///< setprecision() needs it
using namespace std;

// static constant definitions
const int EthernetLink::interframe_gap_bits = 96;
/// Gigabit Ethernet
const sc_time EthernetLink::time_per_bit = sc_time(1, SC_NS);

EthernetLink::EthernetLink(sc_module_name name) : sc_module(name) {
	packets_delivered = 0;
	SC_THREAD (reader_thread);
}

EthernetLink::~EthernetLink() {
	// Debug info
	std::cout << name() << " sent out " << packets_delivered << " packets\n";
}

void EthernetLink::reader_thread() {
	while(true) {
		// block until there is packet to deliver
		IpPacket* packet = in_port->read();

		// wait as long as it takes for the connection to send the whole packet
//		unsigned int bits = max((packet->data_size + ETHERNET_HEADER_LENGTH) * 8, 512) + interframe_gap_bits;
		unsigned int bits =
				(packet->data_size + ETHERNET_HEADER_LENGTH) * 8 > 512 ?
						(packet->data_size + ETHERNET_HEADER_LENGTH) * 8 : 512;
		bits += interframe_gap_bits;
		sc_time wait_time = bits * time_per_bit;

		// push the packet into the management queue, so that it is later reused
		ip_packet_queue->push(packet);

		// statistics
		// packet count
		n_packets_sent++;	// global counter
		packets_delivered++;	// local counter
		// latency
		sc_time latency = sc_time_stamp() - packet->received;
		total_latency += latency;
		if(latency < min_latency)
			min_latency = latency;
		if(latency > max_latency)
			max_latency = latency;

		// Call wait after latency was computed - otherwise packet->received
		// might be overwritten by the time it is read.
		wait(wait_time);
		m_total_transfer_time += wait_time;
	}
}

void EthernetLink::output_load() const {
	cout << name() << " total transfer time: " << m_total_transfer_time << endl;
	cout << name() << fixed << setprecision(1) << " load: transfer "
			<< (m_total_transfer_time) / (sc_time_stamp()) * 100 << "%."
			<< endl;
}
