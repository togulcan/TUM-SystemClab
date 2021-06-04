#include <iomanip>
#include "consumer.h"

void consumer::consumer_proc() {
	unsigned char cnt = 0;
	unsigned char consumed_data;
	while(true) {
		wait();
		if(rd_en.read() == true && valid.read() == true) {
			consumed_data = d_in.read();
			if (consumed_data != cnt) {
				cout << std::setw(9) << sc_time_stamp() << ": '" << name()
							<< "'\tError! Reading " << (int)consumed_data
							<< " from FIFO but expecting " << (int)cnt << "!"
							<< " Terminating simulation." << endl;
				exit(1);
			}
			else {
				cout << std::setw(9) << sc_time_stamp() << ": '" << name()
						<< "'\treading " << (int)consumed_data << " from FIFO"
						<< endl;
			}
			cnt++;
		}
		rd_en.write(fetch.read());
	}
}

// generate a pattern of read actions
void consumer::fetch_trigger() {

	fetch.write(false);
	// initially wait 1 clk cycle
	wait(100, SC_NS);
	while(true) {
		// wait 4 clock cycles
		wait(400, SC_NS);
		fetch.write(true);
		// try to read for 1 cycle
		wait(100, SC_NS);
		fetch.write(false);
		// wait 5 clock cycles
		wait(500, SC_NS);
		fetch.write(true);
		// try to read for 1 cycle
		wait(100, SC_NS);
		fetch.write(false);
		// wait 4 clock cycles
		wait(400, SC_NS);
		fetch.write(true);
		// try to read for 1 cycle
		wait(100, SC_NS);
		fetch.write(false);
		// wait 3 clock cycles
		wait(300, SC_NS);
		fetch.write(true);
		// try to read for 2 cycle
		wait(200, SC_NS);
		fetch.write(false);
		// wait 1 clock cycles
		wait(100, SC_NS);
		fetch.write(true);
		// try to read for 10 cycle
		wait(1000, SC_NS);
		fetch.write(false);
		// wait 2 clock cycles
		wait(200, SC_NS);
		fetch.write(true);
		// try to read for 1 cycle
		wait(100, SC_NS);
		fetch.write(false);
	}
}
