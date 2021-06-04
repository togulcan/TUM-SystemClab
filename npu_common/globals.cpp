#include "systemc.h"
#include "globaldefs.h"

//----------------------------------------------------------------------
// Project-wide global variables
//----------------------------------------------------------------------

/// max number of packets after which the simulation stops
unsigned int MAX_PACKETS = 100;
//unsigned int MAX_PACKETS;

/// number of CPU-s
unsigned int n_cpus = 1;

/// configures the system to use an accelerator or not
bool use_accelerator = false;

/// definition of clocks for Bus, CPUs and Accelerator (if part of the arcitecture)
sc_time CLK_CYCLE_BUS = sc_time(20, SC_NS);
sc_time CLK_CYCLE_CPU = sc_time(10, SC_NS);
sc_time CLK_CYCLE_ACC; // no default value; if accelerator is used this value has to be specified

// control in which modules log information is output during simulation
/* theses defines ar in globaldefs.h
 #define LOG_BUS 0x01
 #define LOG_MEM 0x02
 #define LOG_DMA 0x04
 #define LOG_CPU 0x08
 #define LOG_ACC 0x10
 */
// set the appropriate bits in teh following varaible to enable logging 
// of the respective modules
unsigned short int do_logging = 0;

/***************************************************************************
 There should not be any need to modify code below
 ***************************************************************************/

/// defifintion of processing effort of different processing steps
/// in units of multiples of the respective clock period
unsigned int CPU_VERIFY_HEADER_CYCLES = 50;
unsigned int CPU_DECREMENT_TTL_CYCLES = 5;
unsigned int CPU_UPDATE_CHECKSUM_CYCLES = 30;
unsigned int CPU_IP_LOOKUP_CYCLES = 350;

/// number of mac units, fix for the laboratory system
const unsigned int nMacs = 4;

/// number of packets that can be stored in the memory
unsigned int n_memory_slots = 128;

/// width of bus in bytes
unsigned int bus_width = 8;

// slaves' addresses
/// memory based address
const soc_address_t MEMORY_BASE_ADDRESS = 0x00000000;
/// Address of the read-only register of the DMA module,
/// packet descriptors can be read from here
const soc_address_t PROCESSOR_QUEUE_ADDRESS = 0x10000000;
/// Write-only DMA config register, post a packet_descriptor here to free a memory slot without
/// transferring data.
const soc_address_t DISCARD_QUEUE_ADDRESS = PROCESSOR_QUEUE_ADDRESS;
/// DMA channel 0 write-only config register, write packet_descriptor here to transfer packet.
const soc_address_t OUTPUT_0_ADDRESS = 0x20000000;
/// DMA channel 1 write-only config register, write packet_descriptor here to transfer packet.
const soc_address_t OUTPUT_1_ADDRESS = 0x30000000;
/// DMA channel 2 write-only config register, write packet_descriptor here to transfer packet.
const soc_address_t OUTPUT_2_ADDRESS = 0x40000000;
/// DMA channel 3 write-only config register, write packet_descriptor here to transfer packet.
const soc_address_t OUTPUT_3_ADDRESS = 0x50000000;
/// The address of the accelerator int the system.
const soc_address_t ACCELERATOR_ADDRESS = 0x60000000;

// files
const char lutConfigFile[] = "../config/lut_entries";

const char pcapFile0[] = "../PCAP_samples/p0.pcap";
const char pcapFile1[] = "../PCAP_samples/p1.pcap";
const char pcapFile2[] = "../PCAP_samples/p2.pcap";
const char pcapFile3[] = "../PCAP_samples/p3.pcap";

// statistics
unsigned long long int n_packets_received = 0;
unsigned long long int n_packets_dropped_input_mac = 0;
unsigned long long int n_packets_dropped_output_mac = 0;
unsigned long long int n_packets_dropped_header = 0;
unsigned long long int n_packets_sent = 0;

sc_time max_latency;
sc_time min_latency;
sc_time total_latency;

// array of names for CPUs
char cpu_names[10][5] = { "CPU0", "CPU1", "CPU2", "CPU3", "CPU4", "CPU5",
		"CPU6", "CPU7", "CPU8", "CPU9" };

/**
 * Clear packet counters and set latency variables
 */
void initialize_statistics() {

	n_packets_received = 0;
	n_packets_dropped_input_mac = 0;
	n_packets_dropped_output_mac = 0;
	n_packets_dropped_header = 0;
	n_packets_sent = 0;

	// zero time, so the first latency will be bigger
	max_latency = SC_ZERO_TIME;
	// big enough value, so the first latency will be smaller
	min_latency = sc_core::sc_time(1000000000.0, SC_MS);
	total_latency = SC_ZERO_TIME;
}
