#include <iomanip>
#include "policer.h"

void policer::policer_proc() {

	unsigned short int d_amount;
	int dec;

	sc_time t;
	t = sc_time_stamp() - last_arrival_time;

	dec = t.value() * r_pol;

	last_arrival_time = sc_time_stamp();

	counter = ((counter - dec) < 0) ? 0 : counter - dec;

	cout << std::setw(9) << last_arrival_time << ": '" << name()
			<< "'\tupdated counter: " << std::setw(4) << counter;

	d_amount = size.read();

	if(counter + d_amount > max) {
		discard.write(true);
		cout << "\tnew counter: " << std::setw(4) << counter << "\tdiscard: "
				<< true << endl;
	}
	else {
		counter += d_amount;
		discard.write(false);
		cout << "\tnew counter: " << std::setw(4) << counter << "\tdiscard: "
				<< false << endl;
	}
}
