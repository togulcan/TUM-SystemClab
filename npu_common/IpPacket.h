/**
 * @file	IpPacket.h
 *
 * @date	Apr 18, 2011
 * @author	Miklos Kirilly
 */

#ifndef IPPACKET_H_
#define IPPACKET_H_

#include <systemc>	///< for sc_time
#include "stdint.h"	

using namespace sc_core;

/**
 * Class holding an IPv4 packet.
 * It contains the content of the packet (member array packet_data) along with two other
 * important members:
 * - data_size: the number of valid unsigned chars in packet_data. It is smaller
 * 		than the size of meaningful data starting at the address of an IpPacket object.
 * 		The latter can be computed by<br>
 * 			sizeof(unsigned int) + sizeof(sc_time) + data_size
 * - received: The time when the packet was received at the receive MAC FIFO.
 * 		Used for latency statistics.
 */
class IpPacket {
public:
	//
	// constants
	//

	/// the max. size of a packet, more than enough for an IP packet
	static const unsigned int PACKET_MAX_SIZE = 2000;

	/// minimal length of an IPv4 header in bytes

	static const unsigned int MINIMAL_IP_HEADER_LENGTH = 20;

	//
	// member variables
	//

	/// number of bytes in the array packet_data
	/// @note NOT the size of the whole object, nor the valid content of the whole object.
	///		See class description.

	uint64_t data_size;
	/// time of reception
	sc_time received;

	/// pointer to the packet data (actually an array)
	unsigned char packet_data[PACKET_MAX_SIZE];

	//
	// interface methods
	//

	/// get IP version information
	unsigned char getVersion() const;

	// get IP header length
	unsigned char getHeaderLength() const;

	// get Type of service
	unsigned char getTOS() const;

	/// get total packet length as stored in the IP header
	unsigned short int getTotalLength() const;

	/// get time to live
	unsigned char getTTL() const;
	/// set time to live
	void setTTL(unsigned char newTTL);

	/// get transport layer protocol
	unsigned char getProtocol() const;

	/// get packet checksum
	unsigned short int getChecksum() const;
	/// set packet checksum
	void setChecksum(unsigned short int newChecksum);

	/// get IPv4 source address
	unsigned int getSourceAddress() const;

	/// get IPv4 destination address
	unsigned int getDestAddress() const;

	/// read-only index
	inline unsigned char operator[](int index) const {
		return packet_data[index];
	}

	// constructor
	/// default constructor
	IpPacket() {
	}

private:
	/// read-write index
	inline unsigned char& operator[](int index) {
		return packet_data[index];
	}
};

#endif /* IPPACKET_H_ */
