#include <tlm.h>
#include <string>
#include "reporting.h"

#include "globaldefs.h"
#include "RAM.h"
#include "IoModule.h"
#include "SimpleBusAT.h"
#include "Cpu.h"

using namespace sc_core;

//  command line parsing
#include "argvparser.h"
using namespace CommandLineProcessing;

int sc_main(int argc, char *argv[]);

/**
 * Simulation main function
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return error code
 * @retval 0 if finishes without error
 */
int sc_main(int argc, char *argv[]) {

	/**********************************************************************/
	/*                     Command line parsing                           */
	/**********************************************************************/

	// you can ignore this section, it is used to read in the required
	// parameters from the command line

	ArgvParser cmd;

	// init
	cmd.setIntroductoryDescription(
			"You can set the simulation parameters on the command line with the following switches.");

	//define error codes
	cmd.addErrorCode(0, "Success");
	cmd.addErrorCode(1, "Error");

	cmd.setHelpOption("h", "help", "Print this help");

	cmd.defineOption("verbose", "Output log infomration",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("verbose", "v");

	cmd.defineOption("n_proc", "# of processor in system. Default value: 1",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("n_proc", "n");

	cmd.defineOption("c", "CPU clock period [ns]. Default value: 10",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("c", "cpu");

	cmd.defineOption("b", "Bus and memory clock period [ns]. Default value: 20",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("b", "bus");

	cmd.defineOption("packets",
			"# of packets to be simulated. Default value: 100",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("packets", "p");

	// finally parse and handle return codes (display help etc...)
	int result = cmd.parse(argc, argv);

	if(result != ArgvParser::NoParserError) {
		cout << cmd.parseErrorDescription(result) << endl;
		cout << "Use " << argv[0]
				<< " --help    to get on the command line parameters" << endl;
		exit(1);
	}

	if(cmd.foundOption("verbose")) {
		//unsigned short int do_logging = 0x17;
		//do_logging = 0x10;
		do_logging = (unsigned int)strtol(cmd.optionValue("verbose").c_str(),
				NULL, 16);
		cout << "cmd.optionValue " << cmd.optionValue("verbose").c_str()
				<< endl;
		cout << "do_logging " << do_logging << endl;
	}
	else
		//unsigned short int do_logging = 0;
		do_logging = 0;

	if(cmd.foundOption("n_proc"))
		n_cpus = atoi(cmd.optionValue("n_proc").c_str());
	else
		n_cpus = 1;

	unsigned int nMasters = n_cpus + nMacs;

	if(cmd.foundOption("p"))
		MAX_PACKETS = atoi(cmd.optionValue("p").c_str());
	else
		MAX_PACKETS = 100;

	if(cmd.foundOption("cpu"))
		CLK_CYCLE_CPU = sc_time(atoi(cmd.optionValue("c").c_str()), SC_NS);
	else
		CLK_CYCLE_CPU = sc_time(10, SC_NS);

	if(cmd.foundOption("bus"))
		CLK_CYCLE_BUS = sc_time(atoi(cmd.optionValue("b").c_str()), SC_NS);
	else
		CLK_CYCLE_BUS = sc_time(20, SC_NS);

	unsigned int nSlaves;

	nSlaves = nMacs + 1/*IO module mem. manager*/+ 1/*RAM*/+ 0/*acc*/;

	/*********************************************************************/
	/*                 End of command line parsing                       */
	/*********************************************************************/


	/*********************************************************************/
	/*                           modules                                 */
	/*********************************************************************/

	// system bus
	SimpleBusAT bus("bus", nMasters, nSlaves, bus_width);

	// system memory (RAM)
	RAM target("memory", n_memory_slots * IpPacket::PACKET_MAX_SIZE, 4);

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
			<< "\n\tload statistics\n"
			<< "===================================================================="
			<< endl;
	for(unsigned int i = 0; i < n_cpus; i++) {
		cpus[i]->output_load();
	}
	bus.output_load();

	cout
			<< "===================================================================="
			<< "\n\tpacket statistics\n"
			<< "===================================================================="
			<< endl;
	cout << "n_packets_received = " << n_packets_received
			<< "\nn_packets_dropped_input_mac = " << n_packets_dropped_input_mac
			<< "\nn_packets_sent = " << n_packets_sent << endl << endl;

	cout << "latency:\n\tmin: " << min_latency << "\n\tmax: " << max_latency
			<< "\n\tavg: " << total_latency / n_packets_sent << endl;

	/**********************************************************************/
	/*                            cleanup                                 */
	/**********************************************************************/
	// delete dynamically allocated processors
	for(unsigned int i = 0; i < n_cpus; i++) {
		delete cpus[i];
	}
	return 0;
}
