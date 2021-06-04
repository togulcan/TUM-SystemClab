#include <iomanip>
#include "data_gen.h"

void data_gen::generate() {

	unsigned short int iat;
	short unsigned int bytes;

	iat = rand() % 10;

	dat.write(0);

	while(true) {
		wait(iat, SC_US);
		bytes = (64 + rand() % 1436);
		iat = (bytes + rand() % (6045 - bytes)) / 155;

		dat.write(bytes);
		cout << std::setw(9) << sc_time_stamp() << ": '" << name()
				<< "'\tnumber of bytes: " << std::setw(4) << bytes << endl;
	}
}
