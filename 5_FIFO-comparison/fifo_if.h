
#ifndef __FIFO_IF
#define __FIFO_IF

#include "systemc.h"

class fifo_if: public sc_interface{
public:
	virtual bool read_fifo(unsigned char *data, unsigned int &count) = 0;
	virtual bool write_fifo(unsigned char *data, unsigned int &count) = 0;
};

#endif // __FIFO_IF
