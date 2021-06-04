/**
 * @file	globaldefs.h
 * The extern variables are instantiated in main.cpp.
 *
 * @date	Apr 18, 2011
 * @author	Miklos Kirilly
 */

#ifndef GLOBALDEFS_H_
#define GLOBALDEFS_H_

#include "systemc"
using namespace sc_core;

// global varaible determines in which modules log information is output
#define LOG_BUS 0x01
#define LOG_MEM 0x02
#define LOG_DMA 0x04
#define LOG_CPU 0x08
#define LOG_ACC 0x10

extern unsigned short int do_logging;

// determines how many packets should be simulated
extern unsigned int MAX_PACKETS;

/// configures the system to use an accelerator or not
extern bool use_accelerator;

// types
/// 32-bit address space
typedef unsigned int soc_address_t;

/// number of CPUs
extern unsigned int n_cpus;

// array of names for CPUs
extern char cpu_names[][5];

/// number of slavess
extern unsigned int nSlaves;

/// number of masters
extern unsigned int nMasters;

/// width of bus in bytes
extern unsigned int bus_width;

/// number of packets that can be stored in the memory
extern unsigned int n_memory_slots;

/// number of MACs, i.e.  Ports
extern const unsigned int nMacs;

//-------------------------------------------------------------------------------
// timing
//-------------------------------------------------------------------------------
/// inverse of bus clock frequency
extern sc_time CLK_CYCLE_BUS;
/// inverse of CPU clock frequency
extern sc_time CLK_CYCLE_CPU;
/// inverse of accelerator frequency
extern sc_time CLK_CYCLE_ACC;

/// variables specify the number of clock cycles needed for processing
extern unsigned int CPU_VERIFY_HEADER_CYCLES;
extern unsigned int CPU_DECREMENT_TTL_CYCLES;
extern unsigned int CPU_UPDATE_CHECKSUM_CYCLES;
extern unsigned int CPU_IP_LOOKUP_CYCLES;

/// speed of the Ethernet links in Mbps
extern unsigned int ethernet_speed;

//-------------------------------------------------------------------------------
// addresses
//-------------------------------------------------------------------------------
/// memory based address
extern const soc_address_t MEMORY_BASE_ADDRESS;
/// Address of the read-only register of the DMA module,
/// packet descriptors can be read from here
extern const soc_address_t PROCESSOR_QUEUE_ADDRESS;
/// Write-only DMA config register, post a packet_descriptor here to free a memory slot without
/// transferring data.
extern const soc_address_t DISCARD_QUEUE_ADDRESS;
/// DMA channel 0 write-only config register, write packet_descriptor here to transfer packet.
extern const soc_address_t OUTPUT_0_ADDRESS;
/// DMA channel 1 write-only config register, write packet_descriptor here to transfer packet.
extern const soc_address_t OUTPUT_1_ADDRESS;
/// DMA channel 2 write-only config register, write packet_descriptor here to transfer packet.
extern const soc_address_t OUTPUT_2_ADDRESS;
/// DMA channel 3 write-only config register, write packet_descriptor here to transfer packet.
extern const soc_address_t OUTPUT_3_ADDRESS;
/// The address of the accelerator int the system.
extern const soc_address_t ACCELERATOR_ADDRESS;

//-------------------------------------------------------------------------------
// struct to hold the important parameters of requesting routing table lookup
// when using the accelerator
//-------------------------------------------------------------------------------
typedef struct LookupRequ {
	/// destination address of the IP packet
	unsigned int destAddress;

	/// Unique ID identifying the processor, it has to be the same number as
	/// the index of IRQ line of the processor in Accelerator::irq.
	unsigned int processorId;

	friend std::ostream& operator<<(std::ostream& o, const LookupRequ& req) {
		o << "processor: " << req.processorId << ", address: "
				<< req.destAddress;
		return o;
	}
} LookupRequest;

//-------------------------------------------------------------------------------
// config files
//-------------------------------------------------------------------------------
extern const char lutConfigFile[];
extern const char pcapFile0[];
extern const char pcapFile1[];
extern const char pcapFile2[];
extern const char pcapFile3[];

//--------------------------------------------------------------------------------
// statistics
//--------------------------------------------------------------------------------

extern unsigned long long int n_packets_received;
extern unsigned long long int n_packets_dropped_input_mac;
extern unsigned long long int n_packets_dropped_output_mac;
extern unsigned long long int n_packets_dropped_header;
extern unsigned long long int n_packets_sent;

extern sc_time max_latency;
extern sc_time min_latency;
extern sc_time total_latency;

void initialize_statistics();

#endif /* GLOBALDEFS_H_ */
