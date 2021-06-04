#include "stimulus.h"

void stimulus::c_gen() {

	bool takt = false;

	while(true) {
		takt = !takt;
		clk.write(takt);
		wait(5, SC_NS);
	}
}

void stimulus::r_gen() {

	bool rst = false;
	rst_n.write(!rst);
	wait(17, SC_NS);
	rst = true;
	rst_n.write(!rst);
	wait(10, SC_NS);
	rst = false;
	rst_n.write(!rst);
	wait(SC_ZERO_TIME);
	// no while loop as we only reset once at the start
}
