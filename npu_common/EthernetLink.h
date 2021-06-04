/**
 * @file	EthernetLink.h
 *
 * @date	May 10, 2011
 * @author	Miklos Kirilly
 */

#ifndef ETHERNETLINK_H_
#define ETHERNETLINK_H_

#include <systemc>
#include <queue>
#include "IpPacket.h"
#include "globaldefs.h"
using namespace sc_core;

/**
 * @class EthernetLink
 * Simple emulator of an Ethernet connection.
 */
SC_MODULE (EthernetLink) {
	// static members
public:
	/// length of interframe gap in bits
	static const int interframe_gap_bits;

	/// time required to transmit a bit over Gbit Ethernet
	static const sc_time time_per_bit;

	/// Ethernet header size in bytes
	static const unsigned int ETHERNET_HEADER_LENGTH = 14;

	// instance members
public:

	/// port accessing the MAC output FIFO
	sc_port<sc_fifo_in_if<IpPacket *> > in_port;

	/// pointer to a packet management queue
	std::queue<IpPacket *> *ip_packet_queue;
private:
	unsigned int packets_delivered;

	sc_time m_total_transfer_time;
public:
	/// print load
	void output_load() const;

	/// constructor
	SC_CTOR (EthernetLink);
	/// print out no. of packets sent through this link
	~EthernetLink();

private:
	void reader_thread();
};

#endif /* ETHERNETLINK_H_ */
