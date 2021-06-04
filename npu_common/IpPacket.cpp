/**
 * @file	IpPacket.cpp
 *
 * @date	Apr 18, 2011
 * @author	Miklos Kirilly
 */

#include "IpPacket.h"

// get IP version information
unsigned char IpPacket::getVersion() const {
	return (packet_data[0] & 0xF0) >> 4;
}

// get IP header length in 4-byte words
unsigned char IpPacket::getHeaderLength() const {
	return (packet_data[0] & 0x0F);
}

// get Type of service
unsigned char IpPacket::getTOS() const {
	return packet_data[1];
}

// get total packet length in bytes
unsigned short int IpPacket::getTotalLength() const {
	return (packet_data[2] << 8) + packet_data[3];
}

// get time to live
unsigned char IpPacket::getTTL() const {
	return packet_data[8];
}

// set time to live
void IpPacket::setTTL(unsigned char newTTL) {
	packet_data[8] = newTTL;
}

// get transport layer protocoll
unsigned char IpPacket::getProtocol() const {
	return packet_data[9];
}

unsigned short int IpPacket::getChecksum() const {
	return (packet_data[10] << 8) + packet_data[11];
}

void IpPacket::setChecksum(unsigned short int newChecksum) {
	packet_data[10] = static_cast<unsigned char>(newChecksum >> 8);
	packet_data[11] = static_cast<unsigned char>(newChecksum & 0x00FF);
}

unsigned int IpPacket::getSourceAddress() const {
	return (packet_data[12] << 24) + (packet_data[13] << 16)
			+ (packet_data[14] << 8) + packet_data[15];
}

unsigned int IpPacket::getDestAddress() const {
	return (packet_data[16] << 24) + (packet_data[17] << 16)
			+ (packet_data[18] << 8) + packet_data[19];
}
