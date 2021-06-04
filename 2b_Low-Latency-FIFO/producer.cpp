#include <iomanip>
#include "producer.h"

void producer::producer_proc() {
	unsigned int prod_data = 0;
	while(true) {
		wait();
		// check if last word has been written
		if(wr_en.read() == true && full.read() == false) {
			prod_data++;
		}
		// check if word should be written
		if(send.read() == true) {
			cout << std::setw(9) << sc_time_stamp() << ": '" << name()
					<< "'\tattempting to write " << (int)prod_data << " to FIFO"
					<< endl;
			d_out.write(prod_data);
		}
		// update 'wr_en' with 'send'
		wr_en.write(send.read());
	}
}

// generate a pattern of write actions
void producer::send_trigger() {

	send.write(false);
	// initially wait 1 clk cycle
	wait(100, SC_NS);
	while(true) {
		// wait 2 clock cycles
		wait(200, SC_NS);
		send.write(true);
		// try to write for 1 cycle
		wait(100, SC_NS);
		send.write(false);
		// wait 2 clock cycles
		wait(200, SC_NS);
		send.write(true);
		// try to write for 1 cycle
		wait(100, SC_NS);
		send.write(false);
		// wait 1 clock cycle
		wait(100, SC_NS);
		send.write(true);
		// try to write for 1 cycle
		wait(100, SC_NS);
		send.write(false);
		// wait 2 clock cycles
		wait(200, SC_NS);
		send.write(true);
		// try to write for 1 cycle
		wait(100, SC_NS);
		send.write(false);
		// wait 2 clock cycles
		wait(200, SC_NS);
		send.write(true);
		// try to write for 10 cycles
		wait(1000, SC_NS);
		send.write(false);
		// wait 1 clock cycle
		wait(100, SC_NS);
		send.write(true);
		// try to write for 1 cycle
		wait(100, SC_NS);
		send.write(false);
		// wait 18 clock cycles, then start from the top
		wait(1800, SC_NS);
	}
}
