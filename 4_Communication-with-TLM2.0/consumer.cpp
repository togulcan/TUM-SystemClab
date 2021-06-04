#include <iostream>
#include <iomanip>
#include "consumer.h"
#include "systemc.h"
#include "tlm.h"

// to make things more readable ...
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

void consumer::consumer_proc() {

	int data_len; // number of bytes that should be written [1; 16]
	unsigned char cnt = 0;

	// setup generic payload with an appropriate buffer for the data to be read
	tlm_generic_payload trans;
	unsigned char consum_data[16];
	trans.set_data_ptr(consum_data);

	tlm_sync_enum tlm_resp;
	tlm_response_status tlm_stat;
	sc_time delay;
	tlm_phase phase;

	while(true) {
		// wait until new read request is issued
		wait(fetch_event);

		// generate read request of random lenght between 1 and 16 bytes
		data_len = 1 + rand() % 16;

		// prepare transaction
		trans.set_command(TLM_READ_COMMAND);
		trans.set_data_length(data_len);
		trans.set_response_status(TLM_GENERIC_ERROR_RESPONSE);
		delay = SC_ZERO_TIME;
		phase = BEGIN_REQ;

		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\twants to read " << data_len << " bytes." << endl;

		// call transaction
		tlm_resp = consum2fifo_socket->nb_transport_fw(trans, phase, delay);

		// check response
		if(tlm_resp != TLM_UPDATED || phase != END_REQ) {
			cout << std::setw(9) << sc_time_stamp() << ": '" << name()
					<< "'\tprotocol error! "
					<< "Read request not completed appropriately!" << endl;
			exit(1);
		}

		// now we wait for the response
		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\twaiting until transaction is finished." << endl << endl;
		wait(response_event);

		// now response via nb_transport_bw has arrived
		// evaluate return status of transaction
		tlm_stat = trans.get_response_status();
		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\ttransaction finished ";

		if(tlm_stat == TLM_OK_RESPONSE)
			cout << "successfully. ";
		else if(tlm_stat == TLM_BURST_ERROR_RESPONSE)
			cout << "not or only in part successfully. ";
		else {
			cout << "unsuccessfully with unhandled return status: "
					<< tlm_stat << endl;
			exit(1);
		}
		data_len = trans.get_data_length();
		cout << data_len << " bytes were read." << endl;

		// check values
		for(int i = 0; i < data_len; i++) {
			if(consum_data[i] != ((cnt + i) % 256)) {
				cout << "\tError: Data mismatch. Expected " << hex
						<< (cnt + i) % 256 << " but read "
						<< (int)consum_data[i]
						<< " Terminating simulation." << endl;
				exit(1);
			}
		}

		// update counter to check for stream of increasing numbers
		cnt = (cnt + data_len) % 256;
	} // end while true
} // end consumer

// implementation of nb_transport_bw
tlm_sync_enum consumer::nb_transport_bw(
		tlm_generic_payload &payload,	// ref to Generic Payload
		tlm_phase &phase,				// ref to phase
		sc_time &delay_time				// ref to delay time
		)
{

	if(phase == BEGIN_RESP) {
		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\tread confirmation coming." << endl << endl;
	}
	else {
		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\tprotocol error! Read not confirmed correctly!" << endl;
		exit(1);
	}

	// increase delay time by a value corresponding to transfer time of
	// confirmation, assumed to be 1 clock cycle with 100 ns cycle time
	delay_time += sc_time(100, SC_NS);

	// consume process should go on after time needed for the confirmation
	response_event.notify(delay_time);

	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}

// trigger for read transactions
void consumer::fetch_trigger() {

	while(true) {
		wait(1200, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(800, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(1000, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(800, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(600, SC_NS);
		fetch_event.notify(0, SC_NS);
		wait(1200, SC_NS);
		fetch_event.notify(0, SC_NS);
	}
}
