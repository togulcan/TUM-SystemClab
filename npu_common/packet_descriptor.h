/**
 * @file	m_packet_descriptor.h
 *
 * @date	May 10, 2011
 * @author	Miklos Kirilly
 */

#ifndef PACKET_DESCRIPTOR_H_
#define PACKET_DESCRIPTOR_H_

struct packet_descriptor {
	soc_address_t baseAddress;
	unsigned int size;
};

inline std::ostream& operator<<(std::ostream& o,
		const packet_descriptor& desc) {
	o << "packet @" << std::hex << desc.baseAddress << ".."
			<< desc.baseAddress + desc.size << std::dec;
	return o;
}

#endif /* PACKET_DESCRIPTOR_H_ */
