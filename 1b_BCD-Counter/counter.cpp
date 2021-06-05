#include "counter.h"

void counter::count() {
	while(true){
		wait(clk.posedge_event());
		if (cnt_int > 99)
			cnt_int = 0;
		else
			cnt_int++;
		if (!rst_n)
			cnt_int = 0;
		cnt.write(cnt_int);
	}
}
