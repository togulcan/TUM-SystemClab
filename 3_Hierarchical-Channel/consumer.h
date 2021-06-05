#ifndef __CONSUMER
#define __CONSUMER

// include the necessary header files
#include "systemc.h"
#include "fifo_2.h"

SC_MODULE(consumer) {

public:
	// declare the port of the module
	sc_port<fifo_if> consum2fifo_port;

private:
	// event for communication between consumer() and fetch_trigger()
	sc_event fetch_event;

private:
	// process declaration
	void consumer_proc();
	void fetch_trigger();

public:
	// constructor
	SC_CTOR(consumer) {
		// process registration
		SC_THREAD(consumer_proc);
		SC_THREAD(fetch_trigger);
	}
};

#endif // __CONSUMER
