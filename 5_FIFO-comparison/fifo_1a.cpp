#include <iomanip>
#include "fifo_1a.h"

fifo_1a::fifo_1a(sc_module_name name, unsigned int fifo_size) : fifo_size(fifo_size) {
	// register processes
	SC_METHOD(write_fifo);
    sensitive << clk.pos();
	SC_METHOD(read_fifo);
    sensitive << clk.pos();

	SC_METHOD(set_flags);
	sensitive << rd_ptr << wr_ptr;

	// initialization of member variables/ports
	fifo_data = new unsigned char[fifo_size];
	rd_ptr.write(0);
	wr_ptr.write(0);
	fill_level = 0;

	// initialize output ports
    full.initialize(false);
    d_out.initialize(0);
    empty.initialize(false);
}

void fifo_1a::write_fifo() {
    unsigned int pos = wr_ptr.read();
    if(wr_en.read() == true && full.read() == false){
        *(fifo_data + pos) = d_in.read();
		pos++;
        fill_level++;
        if(pos == fifo_size)
            wr_ptr.write(0);
        else
            wr_ptr.write(pos);
    }
}

void fifo_1a::read_fifo() {
    unsigned int pos = rd_ptr.read();
    if(rd_en.read() == true && empty.read() == false){
        unsigned char data = *(fifo_data + pos);
        d_out.write(data);
        pos++;
        fill_level--;
        if(pos == fifo_size)
            rd_ptr.write(0);
        else
            rd_ptr.write(pos);
    }
}

void fifo_1a::set_flags() {
	// set 'full' & 'empty' flags according to fill level
    next_trigger();
    if(fill_level == 0)
        empty.write(true);
    else
        empty.write(false);

    if(fifo_size == fill_level)
        full.write(true);
    else
        full.write(false);
}
