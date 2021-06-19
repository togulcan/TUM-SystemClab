#include "systemc.h"
#include "tlm.h"
#include "reporting.h"

#include "globaldefs.h"
#include "RAM.h"
#include "IoModule.h"
#include "SimpleBusAT.h"
#include "Cpu.h"

using namespace sc_core;

int sc_main(int argc, char *argv[]) {

	// set some global variables; see $HOME/npu_common/globaldefs.h
	do_logging = 0x0;
	MAX_PACKETS = 20;

	/*********************************************************************/
	/*                           modules                                 */
	/*********************************************************************/
	unsigned int nMasters = n_cpus + nMacs;
	unsigned int nSlaves = nMacs + 1/*IO module mem. manager*/+ 1/*RAM*/;

	// system bus
	SimpleBusAT bus("bus", nMasters, nSlaves, 8/*bus width in bytes*/);

	// system memory (RAM)
	RAM target("memory_target", n_memory_slots * IpPacket::PACKET_MAX_SIZE, 4);

	// Ethernet MAC + DMA
	IoModule mac_io_module("io_module");

	// an array of Cpu pointers
	Cpu* cpus[n_cpus];
	for(unsigned int i = 0; i < n_cpus; i++) {
		cpus[i] = new Cpu(cpu_names[i]);
	}

	/**********************************************************************/
	/*                           wiring                                   */
	/**********************************************************************/

	// interrupt lines
	sc_signal<bool> dma_irq;

	// --------------- BUS MASTERS -------------------
	// DMA engine to bus
	mac_io_module.dma_ch_0.initiator_socket(bus.target_socket[0]);
	mac_io_module.dma_ch_1.initiator_socket(bus.target_socket[1]);
	mac_io_module.dma_ch_2.initiator_socket(bus.target_socket[2]);
	mac_io_module.dma_ch_3.initiator_socket(bus.target_socket[3]);

	// processors to bus and interrupt
	for(unsigned int i = 0; i < n_cpus; i++) {
		// connect master socket to the bus
		cpus[i]->initiator_socket(bus.target_socket[i + nMacs]);
		// connect IRQ lines
		cpus[i]->packetReceived_interrupt(dma_irq);
	}

	// --------------- BUS SLAVES --------------------
	// RAM to bus
	bus.initiator_socket[0](target.m_memory_socket);
	// DMA to bus
	bus.initiator_socket[1](mac_io_module.memory_manager.target_socket);
	bus.initiator_socket[2](mac_io_module.dma_ch_0.target_socket);
	bus.initiator_socket[3](mac_io_module.dma_ch_1.target_socket);
	bus.initiator_socket[4](mac_io_module.dma_ch_2.target_socket);
	bus.initiator_socket[5](mac_io_module.dma_ch_3.target_socket);

	// DMA to interrupt line
	mac_io_module.dma_irq(dma_irq);

	initialize_statistics();

	/**********************************************************************/
	/*                       start simulation                             */
	/**********************************************************************/
	sc_start(); // run as long as needed for the specified number of packets

	/**********************************************************************/
	/*                       print statistics                             */
	/**********************************************************************/

	cout
			<< "===================================================================="
			<< "\n\tpacket statistics\n"
			<< "===================================================================="
			<< endl;
	cout << "n_packets_received = " << n_packets_received
			<< "\nn_packets_sent = " << n_packets_sent << endl << endl;

	/**********************************************************************/
	/*                            cleanup                                 */
	/**********************************************************************/
	// delete dynamically allocated processors
	for(unsigned int i = 0; i < n_cpus; i++) {
		delete cpus[i];
	}

	return 0;
}
