#include <tlm.h>
#include <string>
#include "reporting.h"

#include "globaldefs.h"
#include "RAM.h"
#include "IoModule.h"
#include "SimpleBusAT.h"
#include "Cpu.h"
#include "Accelerator.h"

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

	cmd.defineOption("a",
			"Accelerator clock period [ns]. If option is omitted accel will not be instanitated",
			ArgvParser::OptionRequiresValue);
	cmd.defineOptionAlternative("a", "accel");

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

	if(cmd.foundOption("accel")) {
		use_accelerator = true;
		nSlaves = nMacs + 1/*IO module mem. manager*/+ 1/*RAM*/+ 1/*acc*/;
		CLK_CYCLE_ACC = sc_time(atoi(cmd.optionValue("a").c_str()), SC_NS);
		cout << "use accel = " << use_accelerator << ", CLK_CYCLE_ACC= "
				<< CLK_CYCLE_ACC << endl;
	}
	else {
		use_accelerator = false;
		nSlaves = nMacs + 1/*IO module mem. manager*/+ 1/*RAM*/+ 0/*acc*/;
		CLK_CYCLE_ACC = sc_time(10, SC_NS);
	}

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

	Accelerator *accelerator;
	if(use_accelerator) {
		// ############# COMPLETE THE FOLLOWING SECTION ############# //
		// Write down the Instantiation of the Parameterized Accelerator module
		// with IP Lookup time as parameter

		// ####################### UP TO HERE ####################### //
	}

	/**********************************************************************/
	/*                           wiring                                   */
	/**********************************************************************/

	// interrupt lines
	sc_signal<bool> dma_irq;
	sc_signal<bool> acc_irq[n_cpus];

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
		cpus[i]->lookupReady_interrupt(acc_irq[i]);
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

	// Accelerator to bus
	if(use_accelerator)
		bus.initiator_socket[6](accelerator->target_socket);

	// DMA to interrupt line
	mac_io_module.dma_irq(dma_irq);

	// accelerator to interrupt lines
	if(use_accelerator)
		for(unsigned int i = 0; i < n_cpus; i++) {
			accelerator->irq[i](acc_irq[i]);
		}

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
	sc_time ref_time = sc_time_stamp();

	double mean_proc = 0.0;
	double mean_trans = 0.0;

	for(unsigned int i = 0; i < n_cpus; i++) {
		cpus[i]->output_load();
		mean_proc += (cpus[i]->total_processing_time) / ref_time * 100.0;
		mean_trans += (cpus[i]->total_transfer_time) / ref_time * 100.0;
	}

	cout << "mean CPU processing load: " << mean_proc / n_cpus << " %" << endl;
	cout << "mean CPU transfer load: " << mean_trans / n_cpus << " %" << endl;

	if(use_accelerator)
		accelerator->output_load();

	bus.output_load();

	cout
			<< "===================================================================="
			<< "\n\tpacket statistics\n"
			<< "===================================================================="
			<< endl;
	cout << "n_packets_received = " << n_packets_received
			<< "\nn_packets_dropped_input_mac = " << n_packets_dropped_input_mac
			<< "\nn_packets_sent = " << n_packets_sent << endl
			<< "packet rate = "
			<< n_packets_sent / (ref_time.to_seconds() * 1e3) << " kpps"
			<< endl;

	cout << "latency:\n\tmin: " << min_latency << "\n\tmax: " << max_latency
			<< "\n\tavg: " << total_latency / n_packets_sent << endl;

	/**********************************************************************/
	/*                            cleanup                                 */
	/**********************************************************************/

	// delete dynamically allocated processors
	for(unsigned int i = 0; i < n_cpus; i++) {
		delete cpus[i];
	}

	if(use_accelerator)
		delete accelerator;

	return 0;
}
