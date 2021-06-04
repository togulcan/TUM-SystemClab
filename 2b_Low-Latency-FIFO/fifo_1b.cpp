#include <iomanip>
#include "fifo_1b.h"

fifo_1b::fifo_1b(sc_module_name name, unsigned int fifo_size) : fifo_size(fifo_size) {
	// register process
    SC_THREAD(read_write_fifo);
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
	unsigned int current_size = 0;
    while(true){
        wait();
        unsigned char rd_pos = rd_ptr.read();
        unsigned char wr_pos = wr_ptr.read();
        if(wr_en.read() == true && full.read() == false){
            unsigned char data = d_in.read();
            if(current_size == 0 && valid.read() == false){
                d_out.write(data);
                valid.write(true);
            }
            else if(current_size == fifo_size)
                full.write(true);
            else{
                full.write(false);
                *(fifo_data + wr_pos) = data;
                wr_ptr.write(++wr_pos);
                current_size++;
                if(wr_pos == fifo_size)
                    wr_ptr.write(0);
            }
        }
        if(rd_en.read() == true && valid.read() == true){
            if(current_size == 0)
                valid.write(false);
            else{
                unsigned char data = *(fifo_data + rd_pos);
                d_out.write(data);
                rd_ptr.write(++rd_pos);
                current_size--;
                if(rd_pos == fifo_size)
                    rd_ptr.write(0);
            }
        }
    }
}
