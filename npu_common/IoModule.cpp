/**
 * @file	IoModule.cpp
 * @author	Miklos Kirilly
 * @date	May 6, 2011
 */

#include "IoModule.h"                           // Top traffic generator & initiator
#include "PcapImporter.h"
#include "reporting.h"                          // reporting macro helpers
//static const char *filename = "IoModule.cpp";	///< filename for reporting

//-----------------------------------------------------------------
// constructor
//-----------------------------------------------------------------
IoModule::IoModule(sc_core::sc_module_name name) :
		sc_module(name),
		memory_manager("memory_manager"),
		dma_ch_0("dma_ch0"),
		dma_ch_1("dma_ch1"),
		dma_ch_2("dma_ch2"),
		dma_ch_3("dma_ch3"),
		importer_0("eth0_in", pcapFile0),
		importer_1("eth1_in", pcapFile1),
		importer_2("eth2_in", pcapFile2),
		importer_3("eth3_in", pcapFile3),
		link_0("eth0_out"),
		link_1("eth1_out"),
		link_2("eth2_out"),
		link_3("eth3_out")
{

	// fill up packet queue
	for(unsigned int i = 0; i < 100; i++) {
		IpPacket* p = new IpPacket();
		packet_queue.push(p);
	}
	//--------------------------------------------------------------
	// bind FIFOs
	//--------------------------------------------------------------
	// Bind ports to mac0_in_fifo between dma_ch_0 and importer
	importer_0.out_port(mac0_in_fifo);
	dma_ch_0.mac_in_port(mac0_in_fifo);

	// Bind ports to mac0_out_fifo between dma_ch_0 and link
	dma_ch_0.mac_out_port(mac0_out_fifo);
	link_0.in_port(mac0_out_fifo);

	// Bind ports to mac1_in_fifo between dma_ch_1 and importer
	importer_1.out_port(mac1_in_fifo);
	dma_ch_1.mac_in_port(mac1_in_fifo);

	// Bind ports to mac1_out_fifo between dma_ch_1 and link
	dma_ch_1.mac_out_port(mac1_out_fifo);
	link_1.in_port(mac1_out_fifo);

	// Bind ports to mac2_in_fifo between dma_ch_2 and importer
	importer_2.out_port(mac2_in_fifo);
	dma_ch_2.mac_in_port(mac2_in_fifo);

	// Bind ports to mac2_out_fifo between dma_ch_2 and link
	dma_ch_2.mac_out_port(mac2_out_fifo);
	link_2.in_port(mac2_out_fifo);

	// Bind ports to mac3_in_fifo between dma_ch_3 and importer
	importer_3.out_port(mac3_in_fifo);
	dma_ch_3.mac_in_port(mac3_in_fifo);

	// Bind ports to mac3_out_fifo between dma_ch_3 and link
	dma_ch_3.mac_out_port(mac3_out_fifo);
	link_3.in_port(mac3_out_fifo);

	//---------------------------------------------------------
	// other connections
	//---------------------------------------------------------

	// Bind IRQ port of submodule to IoModule IRQ port
	memory_manager.new_packet_IT(dma_irq);

	// bind pointers to free address registry in memory_manager
	dma_ch_0.free_memory_addresses = &memory_manager.free_memory_addresses;
	dma_ch_1.free_memory_addresses = &memory_manager.free_memory_addresses;
	dma_ch_2.free_memory_addresses = &memory_manager.free_memory_addresses;
	dma_ch_3.free_memory_addresses = &memory_manager.free_memory_addresses;
	dma_ch_0.packetQueue = &memory_manager.packet_queue;
	dma_ch_1.packetQueue = &memory_manager.packet_queue;
	dma_ch_2.packetQueue = &memory_manager.packet_queue;
	dma_ch_3.packetQueue = &memory_manager.packet_queue;
	// bind all to packet_queue
	importer_0.unused_packets_queue = &packet_queue;
	importer_1.unused_packets_queue = &packet_queue;
	importer_2.unused_packets_queue = &packet_queue;
	importer_3.unused_packets_queue = &packet_queue;
	dma_ch_0.ip_packet_buffer = &packet_queue;
	dma_ch_1.ip_packet_buffer = &packet_queue;
	dma_ch_2.ip_packet_buffer = &packet_queue;
	dma_ch_3.ip_packet_buffer = &packet_queue;
	link_0.ip_packet_queue = &packet_queue;
	link_1.ip_packet_queue = &packet_queue;
	link_2.ip_packet_queue = &packet_queue;
	link_3.ip_packet_queue = &packet_queue;
}

//-----------------------------------------------------------------
// destructor
//-----------------------------------------------------------------
IoModule::~IoModule() {

	IpPacket * p; // pointer to packets that are deleted

	// free all packets in the queue
	while(packet_queue.empty() == false) {
		p = packet_queue.front();
		packet_queue.pop();
		delete p;
	}

	// and the FIFOs
	while(mac0_in_fifo.nb_read(p)) {
		delete p;
	}
	while(mac0_out_fifo.nb_read(p)) {
		delete p;
	}
}

void IoModule::output_load() const {
	importer_0.output_load();
	importer_1.output_load();
	importer_2.output_load();
	importer_3.output_load();

	link_0.output_load();
	link_1.output_load();
	link_2.output_load();
	link_3.output_load();
}
