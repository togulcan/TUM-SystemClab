#include <iomanip>
#include <chrono>
#include "testbench.h"

testbench::testbench(sc_module_name name, unsigned int fifo_size,
		unsigned int min_words, unsigned int rnd_range) :
		fifo_size(fifo_size),
		min_words(min_words),
		rnd_range(rnd_range)
{
	// register processes
	SC_THREAD(testbench_thr);
	SC_THREAD(write_trigger);
	SC_THREAD(read_trigger);
	SC_THREAD(clk_gen);
	SC_THREAD(test_fifo_1a_wr);
	sensitive << clk.pos();
	SC_THREAD(test_fifo_1a_rd);
	sensitive << clk.pos();
	SC_THREAD(test_fifo_1b_wr);
	sensitive << clk.pos();
	SC_THREAD(test_fifo_1b_rd);
	sensitive << clk.pos();
	SC_THREAD(test_fifo_2_wr);
	SC_THREAD(test_fifo_2_rd);
	SC_THREAD(test_fifo_3_wr);
	SC_THREAD(test_fifo_3_rd);

	// initialize ports
	clk.initialize(true);
	fifo_1a_in.initialize(-1);
	fifo_1a_wr_en.initialize(false);
	fifo_1a_rd_en.initialize(false);
	fifo_1b_in.initialize(-1);
	fifo_1b_wr_en.initialize(false);
	fifo_1b_rd_en.initialize(false);

	// register nb_transport_bw functions with sockets
	fifo_3_wr_socket.register_nb_transport_bw(this, &testbench::nb_transport_bw_wr);
	fifo_3_rd_socket.register_nb_transport_bw(this, &testbench::nb_transport_bw_rd);

	// initialize member variables
	test_data = new unsigned char[DATA_SIZE];
	wr_ptr = 0;
	rd_ptr = 0;
	wr_queue = 0;
	rd_queue = 0;
	fifo_1a_data_valid.write(false);
}

void testbench::testbench_thr() {
	using namespace std::chrono;

	high_resolution_clock::time_point before_test, after_test;
	duration<double> test_time;

	// generate random test data
	// the same data is used for all FIFOs
	for(int i = 0; i < DATA_SIZE; i++)
		test_data[i] = rand() % 256;

	// start TEST_FIFO_1A
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' starting TEST_FIFO_1A" << endl;
	before_test = high_resolution_clock::now();
	fifo_test = TEST_FIFO_1A;
	start_test.notify(0, SC_NS);
	// wait for test to finish
	wait(test_finished);
	after_test = high_resolution_clock::now();
	test_time = duration_cast<duration<double>>(after_test - before_test);
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' TEST_FIFO_1A finished. Test time was " << test_time.count()
			<< " seconds." << endl << endl;
	reset();

	// start TEST_FIFO_1B
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' starting TEST_FIFO_1B" << endl;
	before_test = high_resolution_clock::now();
	fifo_test = TEST_FIFO_1B;
	start_test.notify(0, SC_NS);
	// wait for test to finish
	wait(test_finished);
	after_test = high_resolution_clock::now();
	test_time = duration_cast<duration<double>>(after_test - before_test);
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' TEST_FIFO_1B finished. Test time was " << test_time.count()
			<< " seconds." << endl << endl;
	reset();

	// start TEST_FIFO_2
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' starting TEST_FIFO_2" << endl;
	before_test = high_resolution_clock::now();
	fifo_test = TEST_FIFO_2;
	start_test.notify(0, SC_NS);
	// wait for test to finish
	wait(test_finished);
	after_test = high_resolution_clock::now();
	test_time = duration_cast<duration<double>>(after_test - before_test);
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' TEST_FIFO_2 finished. Test time was " << test_time.count()
			<< " seconds." << endl << endl;
	reset();

	// start TEST_FIFO_3
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' starting TEST_FIFO_3" << endl;
	before_test = high_resolution_clock::now();
	fifo_test = TEST_FIFO_3;
	start_test.notify(0, SC_NS);
	// wait for test to finish
	wait(test_finished);
	after_test = high_resolution_clock::now();
	test_time = duration_cast<duration<double>>(after_test - before_test);
	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' TEST_FIFO_3 finished. Test time was " << test_time.count()
			<< " seconds." << endl << endl;
	reset();

	cout << std::setw(12) << sc_time_stamp() << ": '" << name()
			<< "' stopping simulation" << endl;
	sc_stop();
}

// reset between tests
void testbench::reset() {
	wr_ptr = 0;
	rd_ptr = 0;
	wr_queue = 0;
	rd_queue = 0;
}

// clock for TEST_FIFO_1A & TEST_FIFO_1B
void testbench::clk_gen() {
	while(true) {
		wait(start_test);
		while(fifo_test == TEST_FIFO_1A || fifo_test == TEST_FIFO_1B) {
			wait(50, SC_NS);
			clk.write(!clk.read());
		}
	}
}

// trigger writing to fifo
void testbench::write_trigger() {
	int wr_len;
	int available;
	while(true) {
		// wait until test is started
		wait(start_test);
		while(true) {
			// determine number of words to write
			wr_len = rnd_num_words();
			// check how many test words are still available
			available = DATA_SIZE - wr_ptr - wr_queue;
			// increase write queue but never to a higher value than test data
			// is available
			wr_queue += wr_len > available ? available : wr_len;
			// safe guard to not let queue get too large
			if(wr_queue > (unsigned int)(1 << 31)) {
				cout << std::setw(12) << sc_time_stamp() << ": '" << name()
						<< "'\tError! wr_queue too large: " << wr_queue << "!"
						<< endl;
				exit(1);
			}
			// issue write (for tests 2 + 3)
			write_event.notify(0, SC_NS);
			// check if all remaining test data is queued
			if(wr_len < available) {
				// wait for random amount of time
				int rnd = 2 * rnd_num_words();
				wait(100 * rnd, SC_NS);
			}
			else
				break;
		} // inner while(true)
	} // outer while(true)
}

// trigger reading from fifo
void testbench::read_trigger() {
	int rd_len;
	int available;
	while(true) {
		// wait until test is started
		wait(start_test);
		while(true) {
			// determine number of words to read
			rd_len = rnd_num_words();
			// check how many test words are still available
			available = DATA_SIZE - rd_ptr - rd_queue;
			// increase write queue but never to a higher value than test data
			// is available
			rd_queue += rd_len > available ? available : rd_len;
			// safe guard to not let queue get too large
			if(rd_queue > (unsigned int)(1 << 31)) {
				cout << std::setw(12) << sc_time_stamp() << ": '" << name()
						<< "'\tError! rd_queue too large: " << rd_queue << "!"
						<< endl;
				exit(1);
			}
			// issue read (for tests 2 + 3)
			read_event.notify(0, SC_NS);
			// check if all remaining test data is queued
			if(rd_len <= available) {
				// wait for random amount of time
				int rnd = 2 * rnd_num_words();
				wait(100 * rnd, SC_NS);
			}
			else
				break;
		} // inner while(true)
	} // outer while(true)
}


/******************************************************************************/
/*                         TEST_FIFO_1A processes                             */
/******************************************************************************/

// write process
void testbench::test_fifo_1a_wr() {
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_1A) {
			while(wr_ptr < DATA_SIZE) {
				wait();
				// check if last word has been written
				if(fifo_1a_wr_en.read() == true && fifo_1a_full.read() == false) {
					wr_ptr++;
					wr_queue--;
				}
				// check if word should be written
				if(wr_queue > 0) {
					fifo_1a_in.write(test_data[wr_ptr]);
					fifo_1a_wr_en.write(true);
				}
				else
					fifo_1a_wr_en.write(false);
			} // while(wr_ptr < DATA_SIZE)
		} // if(test == TEST_FIFO_1A)
	} // while(true)
}

// read process
void testbench::test_fifo_1a_rd() {
	unsigned char consumed_data;
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_1A) {
			while(rd_ptr < DATA_SIZE) {
				wait();
				if(fifo_1a_data_valid.read() == true) {
					consumed_data = fifo_1a_out.read();
					if (consumed_data != test_data[rd_ptr]) {
						cout << std::setw(12) << sc_time_stamp() << ": '"
								<< name() << "'\tError! Reading "
								<< (int)consumed_data
								<< " from FIFO 1a but expecting "
								<< (int)test_data[rd_ptr] << "!"
								<< " rd_ptr: " << rd_ptr << endl;
						exit(1);
					}
					rd_ptr++;
					rd_queue--;
				}
				if(fifo_1a_rd_en.read() == true &&
						fifo_1a_empty.read() == false &&
						rd_queue > 0)
				{
					fifo_1a_data_valid.write(true);
				}
				else
					fifo_1a_data_valid.write(false);

				if(rd_queue > 1 || (fifo_1a_rd_en.read() == false && rd_queue > 0))
					fifo_1a_rd_en.write(true);
				else
					fifo_1a_rd_en.write(false);
			} // while(rd_ptr < DATA_SIZE)
			// notify testbench_thr that test has finished
			test_finished.notify(0, SC_NS);
		} // if(test == TEST_FIFO_1A)
	} // while(true)
}


/******************************************************************************/
/*                         TEST_FIFO_1B processes                             */
/******************************************************************************/

// write process
void testbench::test_fifo_1b_wr() {
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_1B) {
			while(wr_ptr < DATA_SIZE) {
				wait();
				// check if last word has been written
				if(fifo_1b_wr_en.read() == true && fifo_1b_full.read() == false) {
					wr_ptr++;
					wr_queue--;
				}
				// check if word should be written
				if(wr_queue > 0) {
					fifo_1b_in.write(test_data[wr_ptr]);
					fifo_1b_wr_en.write(true);
				}
				else
					fifo_1b_wr_en.write(false);
			} // while(wr_ptr < DATA_SIZE)
		} // if(test == TEST_FIFO_1B)
	} // while(true)
}

// read process
void testbench::test_fifo_1b_rd() {
	unsigned char consumed_data;
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_1B) {
			while(rd_ptr < DATA_SIZE) {
				wait();
				if(fifo_1b_rd_en.read() == true && fifo_1b_valid.read() == true) {
					consumed_data = fifo_1b_out.read();
					if (consumed_data != test_data[rd_ptr]) {
						cout << std::setw(12) << sc_time_stamp() << ": '"
								<< name() << "'\tError! Reading "
								<< (int)consumed_data
								<< " from FIFO 1b but expecting "
								<< (int)test_data[rd_ptr] << "!"
								<< " rd_ptr: " << rd_ptr << endl;
						exit(1);
					}
					rd_ptr++;
					rd_queue--;
				}
				fifo_1b_rd_en.write(rd_queue > 0);
			} // while(rd_ptr < DATA_SIZE)
			// notify testbench_thr that test has finished
			test_finished.notify(0, SC_NS);
		} // if(test == TEST_FIFO_1B)
	} // while(true)
}


/******************************************************************************/
/*                         TEST_FIFO_2 processes                              */
/******************************************************************************/

// write process
void testbench::test_fifo_2_wr() {
	unsigned char *ptr;
	unsigned int wr_len;
	bool success;
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_2) {
			wait(write_event);
			while(true) {
				ptr = &test_data[wr_ptr];
				// we always try to send as many words as are queued
				wr_len = wr_queue > fifo_size ? fifo_size : wr_queue;
				// call transaction
				success = fifo_2_wr_port->write_fifo(ptr, wr_len);
				// update write queue and write pointer
				wr_queue -= wr_len;
				wr_ptr += wr_len;
				// check if we are finished
				if(wr_ptr == DATA_SIZE)
					break;
				// if write was successful and queue is not empty, write again
				if(success && wr_queue > 0)
					wait(100, SC_NS);
				// if write was not fully successful but no more events will be
				// issued, try again until successful
				else if(!success && wr_ptr + wr_queue == DATA_SIZE)
					wait(100, SC_NS);
				// otherwise, wait for next event
				else
					wait(write_event);
			} // inner while(true)
		} // if(test == TEST_FIFO_2)
	} // outer while(true)
}

// read process
void testbench::test_fifo_2_rd() {
	unsigned char read_data[fifo_size];
	unsigned int rd_len;
	bool success;
	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_2) {
			wait(read_event);
			while(true) {
				// we always try to read as many words as are queued
				rd_len = rd_queue > fifo_size ? fifo_size : rd_queue;
				// call transaction
				success = fifo_2_rd_port->read_fifo(read_data, rd_len);
				// check data
				for(unsigned int i = 0; i < rd_len; i++) {
					if(read_data[i] != test_data[rd_ptr + i]) {
						cout << std::setw(12) << sc_time_stamp() << ": '"
								<< name() << "'\tError! Reading "
								<< (int)read_data[i]
								<< " from FIFO 2 but expecting "
								<< (int)test_data[rd_ptr + i] << "!"
								<< " rd_ptr: " << rd_ptr << endl;
						exit(1);
					}
				}
				// update read queue and read pointer
				rd_queue -= rd_len;
				rd_ptr += rd_len;
				// check if we are finished
				if(rd_ptr == DATA_SIZE)
					break;
				// if read was successful and queue is not empty, read again
				if(success && rd_queue > 0)
					wait(100, SC_NS);
				// if read was not fully successful but no more events will be
				// issued, try again until successful
				else if(!success && rd_ptr + rd_queue == DATA_SIZE)
					wait(100, SC_NS);
				// otherwise, wait for next event
				else
					wait(read_event);
			} // inner while(true)
			// notify testbench_thr that test has finished
			test_finished.notify(0, SC_NS);
		} // if(test == TEST_FIFO_2)
	} // outer while(true)
}


/******************************************************************************/
/*                         TEST_FIFO_3 processes                              */
/******************************************************************************/

// write process
void testbench::test_fifo_3_wr() {
	unsigned int wr_len;
	unsigned int written;

	// setup generic payload
	tlm_generic_payload trans;
	tlm_sync_enum tlm_resp;
	tlm_response_status tlm_stat;
	sc_time delay;
	tlm_phase phase;

	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_3) {
			wait(write_event);
			while(true) {
				// we always try to send as many words as are queued
				wr_len = wr_queue > fifo_size ? fifo_size : wr_queue;
				// prepare transaction
				trans.set_command(TLM_WRITE_COMMAND);
				trans.set_data_length(wr_len);
				trans.set_response_status(TLM_GENERIC_ERROR_RESPONSE);
				trans.set_data_ptr(&test_data[wr_ptr]);
				delay = SC_ZERO_TIME;
				phase = BEGIN_REQ;

				// call transaction
				tlm_resp = fifo_3_wr_socket->nb_transport_fw(trans, phase, delay);
				if(tlm_resp != TLM_UPDATED || phase != END_REQ) {
					cout << std::setw(12) << sc_time_stamp() << ": '" << name()
							<< "'\tprotocol error! "
							<< "Write request not completed appropriately!"
							<< endl;
					exit(1);
				}

				// wait for response
				wait(response_event_wr);
				// now response via nb_transport_bw_wr has arrived
				// evaluate return status of transaction
				// in this case, only TLM_OK_RESPONSE and
				// TLM_BURST_ERROR_RESPONSE are acceptable responses
				tlm_stat = trans.get_response_status();
				if(tlm_stat != TLM_OK_RESPONSE && tlm_stat != TLM_BURST_ERROR_RESPONSE) {
					cout << sc_time_stamp() << ": '" << name()
							<< "'\tUnhandled return status in test_fifo_3_wr!" << endl;
							exit(1);
				}

				// update write queue and read pointer
				written = trans.get_data_length();
				wr_queue -= written;
				wr_ptr += written;

				// check if we are finished
				if(wr_ptr == DATA_SIZE)
					break;
				// if write was successful and queue is not empty, write again
				if(wr_len == written && wr_queue > 0)
					wait(100, SC_NS);
				// if write was not fully successful but no more events will be
				// issued, try again until successful
				else if(wr_len != written && wr_ptr + wr_queue == DATA_SIZE)
					wait(100, SC_NS);
				// otherwise, wait for next event
				else
					wait(write_event);
			}
		} // if(test == TEST_FIFO_3)
	} // outer while(true)
}

// read process
void testbench::test_fifo_3_rd() {
	unsigned int rd_len;
	unsigned int read;

	// setup generic payload with an appropriate buffer for the data to be read
	tlm_generic_payload trans;
	unsigned char read_data[fifo_size];
	trans.set_data_ptr(read_data);
	tlm_sync_enum tlm_resp;
	tlm_response_status tlm_stat;
	sc_time delay;
	tlm_phase phase;

	while(true) {
		wait(start_test);
		if(fifo_test == TEST_FIFO_3) {
			wait(read_event);
			while(true) {
				// we always try to read as many words as are queued
				rd_len = rd_queue > fifo_size ? fifo_size : rd_queue;
				// prepare transaction
				trans.set_command(TLM_READ_COMMAND);
				trans.set_data_length(rd_len);
				trans.set_response_status(TLM_GENERIC_ERROR_RESPONSE);
				delay = SC_ZERO_TIME;
				phase = BEGIN_REQ;

				// call transaction
				tlm_resp = fifo_3_rd_socket->nb_transport_fw(trans, phase, delay);
				if(tlm_resp != TLM_UPDATED || phase != END_REQ) {
					cout << std::setw(12) << sc_time_stamp() << ": '" << name()
							<< "'\tprotocol error! "
							<< "Read request not completed appropriately!"
							<< endl;
					exit(1);
				}

				// wait for response
				wait(response_event_rd);
				// now response via nb_transport_bw_rd has arrived
				// evaluate return status of transaction
				// in this case, only TLM_OK_RESPONSE and
				// TLM_BURST_ERROR_RESPONSE are acceptable responses
				tlm_stat = trans.get_response_status();
				if(tlm_stat != TLM_OK_RESPONSE && tlm_stat != TLM_BURST_ERROR_RESPONSE) {
					cout << std::setw(12) << sc_time_stamp() << ": '" << name()
							<< "'\tUnhandled return status in test_fifo_3_rd!" << endl;
							exit(1);
				}

				read = trans.get_data_length();
				// check values
				for(unsigned int i = 0; i < read; i++) {
					if(read_data[i] != test_data[rd_ptr + i]) {
						cout << std::setw(12) << sc_time_stamp() << ": '"
								<< name() << "'\tError! Reading "
								<< (int)read_data[i]
								<< " from FIFO 3 but expecting "
								<< (int)test_data[rd_ptr + i] << "!"
								<< " rd_ptr: " << rd_ptr << endl;
						exit(1);
					}
				}

				// update read queue and read pointer
				rd_queue -= read;
				rd_ptr += read;

				// check if we are finished
				if(rd_ptr == DATA_SIZE)
					break;
				// if read was successful and queue is not empty, read again
				if(rd_len == read && rd_queue > 0)
					wait(100, SC_NS);
				// if read was not fully successful but no more events will be
				// issued, try again until successful
				else if(rd_len != read && rd_ptr + rd_queue == DATA_SIZE)
					wait(100, SC_NS);
				// otherwise, wait for next event
				else
					wait(read_event);
			} // inner while(true)
			// notify testbench_thr that test has finished
			test_finished.notify(0, SC_NS);
		} // if(test == TEST_FIFO_3)
	} // outer while(true)
}

// bw function for write transaction
tlm_sync_enum testbench::nb_transport_bw_wr(
		tlm_generic_payload &payload,
		tlm_phase &phase,
		sc_time &delay_time
		)
{
	if(phase != BEGIN_RESP) {
		cout << std::setw(12) << sc_time_stamp() << ": '" << name()
				<< "'\tprotocol error! "
				<< "Write response not completed appropriately!"
				<< endl;
		exit(1);
	}

	// increase delay time by a value corresponding to transfer time of
	// confirmation, assumed to be 1 clock cycle with 100 ns cycle time
	delay_time += sc_time(100, SC_NS);
	// producer process should go on after time needed for the confirmation
	response_event_wr.notify(delay_time);
	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}

// bw function for read transaction
tlm_sync_enum testbench::nb_transport_bw_rd(
		tlm_generic_payload &payload,
		tlm_phase &phase,
		sc_time &delay_time
		)
{
	if(phase != BEGIN_RESP) {
		cout << std::setw(12) << sc_time_stamp() << ": '" << name()
				<< "'\tprotocol error! "
				<< "Read response not completed appropriately!"
				<< endl;
		exit(1);
	}

	// increase delay time by a value corresponding to transfer time of
	// confirmation, assumed to be 1 clock cycle with 100 ns cycle time
	delay_time += sc_time(100, SC_NS);
	// consume process should go on after time needed for the confirmation
	response_event_rd.notify(delay_time);
	// finish the transaction (end of 2nd phase)
	phase = END_RESP;
	return TLM_COMPLETED;
}
