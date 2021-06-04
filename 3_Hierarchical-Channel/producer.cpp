#include <iostream>
#include <iomanip>
#include "producer.h"

void producer::producer_proc() {

	unsigned int data_len; // number of bytes that should be written [1; 16]
	unsigned char prod_data[16];
	unsigned char cnt = 0;

	bool success;

	while(true) {
		// synchronize with trigger process
		wait(send_event);

		// generate random data of random length between 1 and 16 bytes
		data_len = 1 + rand() % 16;
		for(unsigned int i = 0; i < data_len; i++)
			*(prod_data + i) = cnt + i;

		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\twants to write " << data_len << " bytes." << endl << endl;

		// call transaction
		success = prod2fifo_port->write_fifo(prod_data, data_len);

		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\ttransaction finished ";
		if(success)
			cout << "successfully. ";
		else
			cout << "not or only in part successfully. ";
		cout << data_len << " bytes were written." << endl << endl;

		// update counter for stream of increasing numbers
		cnt = (cnt + data_len) % 256;
	}
}

// trigger for write transactions
void producer::send_trigger() {

	while(true) {
		wait(300, SC_NS);
		send_event.notify(0, SC_NS);
		wait(800, SC_NS);
		send_event.notify(0, SC_NS);
		wait(200, SC_NS);
		send_event.notify(0, SC_NS);
		wait(200, SC_NS);
		send_event.notify(0, SC_NS);
		wait(200, SC_NS);
		send_event.notify(0, SC_NS);
		wait(400, SC_NS);
		send_event.notify(0, SC_NS);
		wait(1000, SC_NS);
		send_event.notify(0, SC_NS);
		wait(1000, SC_NS);
		send_event.notify(0, SC_NS);
		wait(400, SC_NS);
		send_event.notify(0, SC_NS);
	}
}
