/**
 * @file	Cpu_proc.cpp
 * Processor implementation.
 *
 * @date	Apr 14, 2011
 * @author	Miklos Kirilly
 */

#include "Cpu.h"
#include <iomanip>

using namespace std;

//*********************************************************************
// processing steps
//*********************************************************************
bool Cpu::verifyHeaderIntegrity(const IpPacket& header) const {

	if(header.getHeaderLength() < 5) {
		cout << "Invalid IP header length\n";
		return false;
	}
	else if(header.getTTL() == 0) {
		cout << "TTL=0 - discard\n";
		return false;
	}
	else if(Cpu::calculateChecksum(header) != header.getChecksum()) {
		cout << "Incorrect header checksum - discard\n";
		return false;
	}
	else {
		return true;
	}
}

unsigned int Cpu::makeNHLookup(const IpPacket& header) {
	return m_rt.getNextHop(header.getDestAddress());
}

unsigned int Cpu::decrementTTL(IpPacket& header) {
	unsigned int newTTL = header.getTTL() - 1;
	header.setTTL(newTTL);
	return newTTL;
}

void Cpu::updateChecksum(IpPacket& header) {
	/*
	 * The function only decrements the checksum, since only the TTL was changed
	 * (decremented by one) in this application. In the 16-but checksum calculation
	 * the TTL byte falls into the left side byte, hence the shifting of 1.
	 */
	header.setChecksum(header.getChecksum() + (1 << 8));
}

unsigned short int Cpu::calculateChecksum(const IpPacket& header) const {
	/*
	 * Add up all 16-bit words, except for the checksum field.
	 * It is simpler to subtract the checksum field than to
	 * handle special cases.
	 */

	int checksum = -header.getChecksum();
	for(unsigned int i = 0; i < header.getHeaderLength() * 4; i = i + 2) {
		// get two bytes, shift one of them to higher position
		checksum += (header[i] << 8) + header[i + 1];
	}

	// pack the sum to 16 bits
	checksum = (checksum + (checksum >> 16)) & 0xFFFF;
	// The value calculated above gives 0 when added to the checksum;
	// in order to get the correct value a bitwise negation is applied.
	return ~checksum;
}

void Cpu::output_load() const {
	cout << name() << " total processing time: " << total_processing_time
			<< endl;
	cout << name() << " total transfer time  : " << total_transfer_time << endl;
	cout << name() << fixed << setprecision(1) << " load: processing "
			<< (total_processing_time) / (sc_time_stamp()) * 100
			<< "%, transferring "
			<< (total_transfer_time) / (sc_time_stamp()) * 100 << "%, in sum: "
			<< (total_transfer_time + total_processing_time) / (sc_time_stamp())
					* 100 << "%." << endl;
}
