#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__

#include <tlm.h>
#include <queue>
#include "DmaChannel.h"
#include "PcapImporter.h"
#include "EthernetLink.h"
#include "MemoryManager.h"

using namespace sc_core;
using namespace tlm;

/**
 * @class IoModule
 *
 * IO module of the SoC.
 * It contains 4 MAC units and one dedicated DMA channel for each. Its memory_manager
 * submodule keeps track of which memory slots are free in the RAM, and the DMA channels
 * can only take received packets from the MAC receive queue if there is free RAM space
 * for them.
 *
 * Also, the model of the Ethernet connections is included in this module. The received packets
 * are sent by a PcapImporter module (importer_0 through importer_3), the outbound packets are
 * passed on to the EthernetLink modules by the transmit FIFOs.
 *
 * @note In this model each DMA channel has an own bus socket. This was done for convenience
 * of programming, a real HW implementation would most probably not contain separate driver
 * circuitry, because if a bus is used as an interconnect (and not, for example, a crossbar)
 * only one of them can transact at the same time.
 */
SC_MODULE(IoModule) {
public:
	/// interrupt to signal when a new packet is received by any of the MAC subunits
	sc_out<bool> dma_irq;

	// *******===============================================================******* //
	// *******                  member objects, variables                    ******* //
	// *******===============================================================******* //
	/// central administrative part, keeps track of memory slot allocation
	MemoryManager memory_manager;

	// four DMA channels, each responsible for reading from and writing to an Ethernet MAC FIFO
	DmaChannel dma_ch_0;
	DmaChannel dma_ch_1;
	DmaChannel dma_ch_2;
	DmaChannel dma_ch_3;

private:

	sc_fifo<IpPacket *> mac0_in_fifo; ///< rx fifo of MAC 0
	sc_fifo<IpPacket *> mac0_out_fifo; ///< tx fifo of MAC 0
	sc_fifo<IpPacket *> mac1_in_fifo; ///< rx fifo of MAC 1
	sc_fifo<IpPacket *> mac1_out_fifo; ///< tx fifo of MAC 1
	sc_fifo<IpPacket *> mac2_in_fifo; ///< rx fifo of MAC 2
	sc_fifo<IpPacket *> mac2_out_fifo; ///< tx fifo of MAC 2
	sc_fifo<IpPacket *> mac3_in_fifo; ///< rx fifo of MAC 3
	sc_fifo<IpPacket *> mac3_out_fifo; ///< tx fifo of MAC 3

	bool m_enable_target_tracking; ///< track target timing

	// Wrapper modules to read data from PCAP dump files
	PcapImporter importer_0;
	PcapImporter importer_1;
	PcapImporter importer_2;
	PcapImporter importer_3;

	// Ethernet lines
	EthernetLink link_0;
	EthernetLink link_1;
	EthernetLink link_2;
	EthernetLink link_3;

	/// Manager for IpPacket objects.
	/// @note Used to speed up simulation, not intended to model any HW.
	std::queue<IpPacket *> packet_queue;

	// *******===============================================================******* //
	// *******                           member function                     ******* //
	// *******===============================================================******* //
public:
	void output_load() const;
	// *******===============================================================******* //
	// *******                             constructor                       ******* //
	// *******===============================================================******* //
public:

	/**
	 * @fn IoModule
	 * Initializes submodules.
	 */
	SC_CTOR(IoModule);

	/**
	 * User defined destructor.
	 *
	 * Override default to free memory allocated to dynamically created IpPacket objects
	 */
	~IoModule();
};

#endif /* __IO_MODULE_H__ */
