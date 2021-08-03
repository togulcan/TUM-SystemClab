#include <iomanip>
#include "fifo_1a.h"

fifo_1a::fifo_1a(sc_module_name name, unsigned int fifo_size) : fifo_size(fifo_size) {
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// register processes
	SC_METHOD(write_fifo);
	sensitive << clk.pos();
	SC_METHOD(read_fifo);
	sensitive << clk.pos();

	// ####################### UP TO HERE ####################### //

	SC_METHOD(set_flags);
	sensitive << rd_ptr << wr_ptr;

	// initialization of member variables/ports
	fifo_data = new unsigned char[fifo_size];
	rd_ptr.write(0);
	wr_ptr.write(0);
	fill_level = 0;

	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// initialize output ports
	full.initialize(false);
	d_out.initialize(0);
	empty.initialize(false);

	// ####################### UP TO HERE ####################### //
}

void fifo_1a::write_fifo() {
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	unsigned int pos = wr_ptr.read();
	if(wr_en.read() == true && full.read() == false){
		*(fifo_data + pos) = d_in.read();
		fill_level++;
		wr_ptr.write(++pos % fifo_size);
	}
	// ####################### UP TO HERE ####################### //
}

void fifo_1a::read_fifo() {
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	unsigned int pos = rd_ptr.read();
	if(rd_en.read() == true && empty.read() == false){
		unsigned char data = *(fifo_data + pos);
		d_out.write(data);
		rd_ptr.write(++pos % fifo_size);
		fill_level--;
	}
	// ####################### UP TO HERE ####################### //
}

void fifo_1a::set_flags() {
	// ############# COMPLETE THE FOLLOWING SECTION ############# //
	// set 'full' & 'empty' flags according to fill level
	next_trigger();
	empty.write(fill_level == 0);
	full.write(fill_level == fifo_size);

	// ####################### UP TO HERE ####################### //
}
