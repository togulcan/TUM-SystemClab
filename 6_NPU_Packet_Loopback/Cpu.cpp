#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {

	tlm_phase phase;
	sc_time delay_time;

	while(true) {
		// read new packet descriptor
		wait(packetReceived_interrupt.value_changed_event());

		std::vector<soc_address_t> list{0x20000000, 0x30000000, 0x40000000, 0x50000000};
		soc_address_t port_address;

		while(packetReceived_interrupt.read()) {
			startTransaction(TLM_READ_COMMAND,
			                 0x10000000,
			                 (unsigned char *) &m_packet_descriptor,
			                 sizeof(m_packet_descriptor));
			// Forward the packet descriptor to an arbitrary port
			//soc_address_t port_value = list[rand() % list.size()]; // a random port taken
			port_address = list[rand() % list.size()]; // a random port taken
			startTransaction(TLM_WRITE_COMMAND,
			                 port_address,
			                 (unsigned char *) &m_packet_descriptor,
			                 sizeof(m_packet_descriptor));
		}
	}
}

// startTransaction
void Cpu::startTransaction(tlm_command command, soc_address_t address,
                           unsigned char *data, unsigned int dataSize){
	tlm_phase phase;
	sc_time delay_time;

	// aggressively send the requests
	do{
		delay_time = SC_ZERO_TIME;
		phase = BEGIN_REQ;
		tlm_sync_enum tlm_resp;

		payload.set_command(command);
		payload.set_address(address);
		payload.set_data_ptr(data);
		payload.set_data_length(dataSize);

		tlm_resp = initiator_socket->nb_transport_fw(payload, phase, delay_time);
		wait(transactionFinished_event);

		if(payload.get_response_status() == TLM_INCOMPLETE_RESPONSE){
			continue;
		}

		if(tlm_resp != TLM_UPDATED){
			cout << sc_time_stamp() << " protocol error detected in  Cpu::startTransaction" <<  endl;
			exit(1);
		}

		if(payload.get_response_status() == TLM_OK_RESPONSE)
			break;

	} while(true);
}


// nb_transport_bw: implementation of the backward path callback
tlm_sync_enum Cpu::nb_transport_bw(tlm_generic_payload& transaction,
		tlm_phase& phase, sc_time& delay_time) {
	if(phase != BEGIN_RESP) {
		cerr << "expected BEGIN_RESP but not received" << endl;
		exit(1);
	}

	delay_time += sc_time(CLK_CYCLE_BUS);
	transactionFinished_event.notify(delay_time);
	phase = END_RESP;
	return TLM_COMPLETED;
}

unsigned int Cpu::instances = 0;
