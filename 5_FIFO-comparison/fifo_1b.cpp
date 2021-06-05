#include <iomanip>
#include "fifo_1b.h"

fifo_1b::fifo_1b(sc_module_name name, unsigned int fifo_size) : fifo_size(fifo_size) {
	// register process
	SC_METHOD(read_write_fifo);
	sensitive << clk.pos();

	// initialization of member variables/ports
	fifo_data = new unsigned char[fifo_size];
	rd_ptr.write(0);
	wr_ptr.write(0);
	fill_level = 0;

	// initialize output ports
    d_out.initialize(0);
    valid.initialize(false);
}

void fifo_1b::read_write_fifo() {
    unsigned int rd_pos = rd_ptr.read();
    unsigned int wr_pos = wr_ptr.read();
    if(wr_en.read() == true && full.read() == false){
        unsigned char data = d_in.read();
        if(fill_level == 0 && valid.read() == false){
            d_out.write(data);
            valid.write(true);
        }
		else{
            *(fifo_data + wr_pos) = data;
            wr_ptr.write(++wr_pos);
            fill_level++;
            if(fill_level == fifo_size - 1) full.write(true);
            if(wr_pos == fifo_size) wr_ptr.write(0); // avoid segmentation fault
		}
    }
    if(rd_en.read() == true && valid.read() == true){
        if(fill_level == 0)
            valid.write(false);
        else{
            unsigned char data = *(fifo_data + rd_pos);
	        rd_ptr.write(++rd_pos);
	        d_out.write(data);
	        valid.write(true);
            fill_level--;
            full.write(false);
            if(rd_pos == fifo_size) rd_ptr.write(0); // avoid segmentation fault
        }
    }
}
