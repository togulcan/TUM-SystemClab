#include <iostream>
#include <iomanip>
#include "consumer.h"

void consumer::consumer_proc() {
	unsigned int data_len; // number of bytes that should be read [1; 16]
	unsigned char consum_data[16];
	unsigned char cnt = 0;

	bool success;

	while(true) {
		wait(fetch_event);

		// generate read request of random length between 1 and 16 bytes
		data_len = 1 + rand() % 16;

		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\twants to read " << data_len << " bytes." << endl << endl;
		// call transaction
		success = consum2fifo_port->read_fifo(consum_data, data_len);

		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\ttransaction finished ";
		if(success)
			cout << "successfully. ";
		else
			cout << "not or only in part successfully. ";
		cout << data_len << " bytes were read." << endl;

		// check values
		cout << hex;
		for(unsigned int i = 0; i < data_len; i++) {
			if(consum_data[i] != ((cnt + i) % 256)) {
				cout << std::setw(9) << sc_time_stamp() << ": '" << name()
							<< "'\tError! Reading " << (int)consum_data[i]
							<< " from FIFO but expecting "
							<< (int)((cnt + i) % 256) << "!"
							<< " Terminating simulation." << endl;
				exit(1);
			}
		}
		cout << dec << endl;

		// update counter to check for stream of increasing numbers
		cnt = (cnt + data_len) % 256;
	}
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
