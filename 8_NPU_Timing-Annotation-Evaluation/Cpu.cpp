#include "Cpu.h"
#include "IoModule.h"
#include "reporting.h"

using namespace sc_core;
using namespace tlm;

void Cpu::processor_thread(void) {

	tlm_phase phase;
	sc_time delay_time;

	while(true) {
		wait(packetReceived_interrupt.value_changed_event());


		while(packetReceived_interrupt.read()) {
			// read descriptor
			startTransactionTimingWrapped(TLM_READ_COMMAND,
			                 0x10000000,
			                 (unsigned char *) &m_packet_descriptor,
			                 sizeof(m_packet_descriptor));

			// read header packet
			unsigned int sizeheader = sizeof(sc_time) + sizeof(uint64_t) + IpPacket::MINIMAL_IP_HEADER_LENGTH;
			startTransactionTimingWrapped(TLM_READ_COMMAND,
			                 0x00000000 + m_packet_descriptor.baseAddress,
			                 (unsigned char *) &m_packet_header,
			                 sizeheader);

			MEASURE_PROCESSING_TIME(
					bool integrity = verifyHeaderIntegrity(m_packet_header);
					wait(CPU_VERIFY_HEADER_CYCLES * CLK_CYCLE_CPU);
			)

			if(!integrity){
				discardDescriptorTimingWrapped();
			}
			else{
				MEASURE_PROCESSING_TIME(
						unsigned int nextHop = makeNHLookup(m_packet_header);
						wait(CPU_IP_LOOKUP_CYCLES * CLK_CYCLE_CPU);
				)

				// forward packet descriptor to output
				startTransactionTimingWrapped(TLM_WRITE_COMMAND,
				                 outports[nextHop],
				                 (unsigned char *) &m_packet_descriptor,
				                 sizeof(m_packet_descriptor));

				MEASURE_PROCESSING_TIME(
						decrementTTL(m_packet_header);
						wait(CPU_DECREMENT_TTL_CYCLES * CLK_CYCLE_CPU);
						updateChecksum(m_packet_header);
						wait(CPU_UPDATE_CHECKSUM_CYCLES * CLK_CYCLE_CPU);
				)

				// write header back to memory
				startTransactionTimingWrapped(TLM_WRITE_COMMAND,
				                 0x00000000 + m_packet_descriptor.baseAddress,
				                 (unsigned char *) &m_packet_header,
				                 sizeheader);
			}
		}
	}
}

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

void Cpu::startTransactionTimingWrapped(tlm_command command, soc_address_t address,
                           unsigned char *data, unsigned int dataSize){
	MEASURE_TRANSFER_TIME(
			startTransaction(command, address, data, dataSize);
	)
}

void Cpu::discardDescriptor(){
	startTransaction(TLM_WRITE_COMMAND,
	                 0x10000000,
	                 (unsigned char *) &m_packet_descriptor,
	                 sizeof(m_packet_descriptor));
}

void Cpu::discardDescriptorTimingWrapped(){
	MEASURE_TRANSFER_TIME(
			discardDescriptor();
			)
}



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
