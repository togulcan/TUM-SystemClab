#ifndef __PRODUCER
#define __PRODUCER

// include the necessary header files
#include "systemc.h"
#include "fifo_2.h"

SC_MODULE(producer) {

public:
	// declare the port of the module
	sc_port<fifo_if> prod2fifo_port;

private:
	// event for communication between producer() and send_trigger()
	sc_event send_event;

private:
	// process declaration
	void producer_proc();
	void send_trigger();

public:
	// constructor
	SC_CTOR(producer) {
		// process registration
		SC_THREAD(producer_proc);
		SC_THREAD(send_trigger);
	}
};

#endif // __PRODUCER
