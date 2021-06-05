#ifndef __FIFO_2
#define __FIFO_2

// include the necessary header files
#include "fifo_if.h"

class fifo_2: public fifo_if, public sc_module {
private:
	const unsigned int fifo_size;
	unsigned char *fifo_data;
	unsigned int wr_ptr, rd_ptr;
	unsigned int fill_level;

	// declaration of helper function
	void output_fifo_status();

public:
	// declaration of the interface functions
	bool read_fifo(unsigned char *data, unsigned int &count);
	bool write_fifo(unsigned char *data, unsigned int &count);

	// constructor declaration
	SC_HAS_PROCESS(fifo_2);
	fifo_2(sc_module_name name, unsigned int fifo_size=50);
};

#endif // __FIFO_2
