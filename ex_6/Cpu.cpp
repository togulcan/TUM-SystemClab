#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {

	tlm_phase phase;
	sc_time delay_time;

	while(true) {
		// ############# COMPLETE THE FOLLOWING SECTION ############# //
		// read new packet descriptor

		// ####################### UP TO HERE ####################### //

		// ############# COMPLETE THE FOLLOWING SECTION ############# //
		// Forward the packet descriptor to an arbitrary port

		// ####################### UP TO HERE ####################### //
	}
}

// ############# COMPLETE THE FOLLOWING SECTION ############# //
// startTransaction

// ####################### UP TO HERE ####################### //

// nb_transport_bw: implementation of the backward path callback
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	// ############# COMPLETE THE FOLLOWING SECTION ############# //

	// ####################### UP TO HERE ####################### //
}

unsigned int Cpu::instances = 0;
