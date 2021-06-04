/**
 * @file	SimpleBusAT.cpp
 *
 * @date	May 27, 2011
 * @author	Miklos Kirilly
 */
#include "SimpleBusAT.h"
#include "reporting.h"

//static const char *filename = "SimpleBusAT.cpp"; ///  filename for reporting

using namespace std;
using namespace sc_core;
using namespace tlm;

SC_HAS_PROCESS (SimpleBusAT);
SimpleBusAT::SimpleBusAT(sc_module_name name, unsigned int n_initiators,
		unsigned int n_targets, unsigned int bus_width) :
		sc_module(name), nr_of_initiators(n_initiators), nr_of_targets(
				n_targets), arbitration_time(CLK_CYCLE_BUS), m_bus_width(
				bus_width), mPEQ("requestPEQ") {

	target_socket =
			new tlm_utils::simple_target_socket_tagged<SimpleBusAT>[nr_of_initiators];
	initiator_socket =
			new tlm_utils::simple_initiator_socket_tagged<SimpleBusAT>[nr_of_targets];
	for(unsigned int i = 0; i < nr_of_initiators; ++i) {
		target_socket[i].register_nb_transport_fw(this,
				&SimpleBusAT::nb_transport_fw_tagged, i);
	}
	for(unsigned int i = 0; i < nr_of_targets; ++i) {
		initiator_socket[i].register_nb_transport_bw(this,
				&SimpleBusAT::nb_transport_bw_tagged, i);
	}

	SC_THREAD (RequestThread);

	total_transfer_time = SC_ZERO_TIME;
}

void SimpleBusAT::RequestThread(void) {
	while(true) {
		wait(mPEQ.get_event());

		tlm_generic_payload* trans;
		while((trans = mPEQ.get_next_transaction()) != 0) {
			MEASURE_TRANSFER_TIME(
				switch(mPendingTransactions.find(trans)->second.phase) {
					case BEGIN_REQ:
						// received request from initiator, the bus now forwards it to the target.
						sendToTarget(trans);
						break;
					case END_REQ:
						// request phase finished
						// only in 4-phase AT
						assert(0);
						break;
					case BEGIN_RESP:
						// target started response
						sendToInitiator(trans);
						break;
					case END_RESP:
						// initiator ends response
						// only in 4-phase AT
						assert(0);
						break;
					default:
						cout << "ERROR: '" << name()
								<< "': Illegal phase in event queue from target."
								<< endl;
						assert(false);
						exit(1);
				}
			)
		}
	}
}

tlm_sync_enum SimpleBusAT::nb_transport_fw_tagged(int initiator_id,
		tlm_generic_payload& payload, tlm_phase& phase, sc_time& delay_time) {
// logging	cout << "\tBus: trans " << &payload << " received, phase: " << phase
// logging			<< endl;
	if(do_logging & LOG_BUS)
		cout << sc_time_stamp() << " " << name() << ": trans " << &payload
				<< " received, phase: " << phase << endl;

	if(phase == BEGIN_REQ) {
		addPendingTransaction(payload, 0, initiator_id, phase);

		// annotate delay time: arbitration time
		delay_time += arbitration_time;

		mPEQ.notify(payload, delay_time);
		phase = END_REQ;
	}
	else {
		cout << "ERROR: '" << name()
				<< "': Illegal phase received from initiator." << endl;
		assert(false);
		exit(1);
	}

	return TLM_UPDATED;
}

tlm_sync_enum SimpleBusAT::nb_transport_bw_tagged(int portId,
		tlm_generic_payload& payload, tlm_phase& phase, sc_time& delay_time) {
// logging	cout << "\tBus: trans " << &payload << " sent by target, phase: " << phase
// logging			<< endl;
	if(do_logging & LOG_BUS)
		cout << sc_time_stamp() << " " << name() << ": trans " << &payload
				<< " sent by target, phase: " << phase << endl;

	if(phase != END_REQ && phase != BEGIN_RESP) {
		cerr << sc_time_stamp() << " " << name()
				<< ": ERROR: Illegal phase received from target." << endl;
		assert(false);
		exit(1);
	}
// Update transaction phase in the pending transactions database.
	mPendingTransactions.find(&payload)->second.phase = phase;

	if(phase == BEGIN_RESP) {
		// post transaction to PEQ
		mPEQ.notify(payload, delay_time);
		// Change phase to END_RESP only here, and don't save END_RESP in the
		// database, because the transaction only ended for the target, not for
		// the initiator.
		phase = END_RESP;
	}
// annotate delay time
	delay_time += get_transport_delay(payload.get_data_length());

	return TLM_COMPLETED;
}

void SimpleBusAT::sendToTarget(tlm_generic_payload* payload_ptr) {

// address translation for the target side
	unsigned int portId = decode(payload_ptr->get_address());
	assert(portId < nr_of_targets);
	simple_initiator_socket_tagged < SimpleBusAT > *decodeSocket =
			&initiator_socket[portId];
	payload_ptr->set_address(
			payload_ptr->get_address() & getAddressMask(portId));

// Fill in the destination port
	PendingTransactionsIterator it = mPendingTransactions.find(payload_ptr);
	assert(it != mPendingTransactions.end());
	it->second.to = decodeSocket;

// Use reference for phase, so that it is automatically
// updated in mPendingTransactions as well.
	tlm_phase& phase = it->second.phase;
	sc_time t = SC_ZERO_TIME;

// logging	cout << "\tBus: trans " << payload_ptr << " sent to target " << portId
// logging			<< ", phase: " << report::print(phase) << endl;
	if(do_logging & LOG_BUS)
		cout << sc_time_stamp() << " " << name() << ": trans " << payload_ptr
				<< " sent to target " << portId << ", phase: "
				<< report::print(phase) << endl;

// FIXME: No limitation on number of pending transactions
//        All targets (that return false) must support multiple transactions
	tlm_sync_enum sync = (*decodeSocket)->nb_transport_fw(*payload_ptr, phase,
			t);
	switch(sync) {
		case TLM_ACCEPTED:
		case TLM_UPDATED:
			// Transaction not yet finished
			if(phase == END_REQ) {
				// Request phase finished, but response phase not yet started
				wait(t); // wait the required time
// logging			cout << "\tBus waiting " << t << "\n";
				if(do_logging & LOG_BUS)
					cout << sc_time_stamp() << " " << name() << ": waiting "
							<< t << "\n";

			}
			else { // END_RESP
				assert(0);
				exit(1);
			}
			break;

		case TLM_COMPLETED:
			// Transaction finished - early completion
			// send to initiator
			mPEQ.notify(*payload_ptr, t);

			// reset to destination port (we must not send END_RESP to target)
			it->second.to = 0;

			wait(t);
			break;

		default:
			assert(0);
			exit(1);
	};
}

void SimpleBusAT::sendToInitiator(tlm_generic_payload* payload_ptr) {
// find the connection info for the transaction
	PendingTransactionsIterator it = mPendingTransactions.find(payload_ptr);
// mPendingPransactions.end() would mean there's no entry in the map for the transaction.
// Very serious error.
	assert(it != mPendingTransactions.end());

// address back-translation for the initiator side
	unsigned int portId = ((unsigned long int)it->second.to
			- (unsigned long int)initiator_socket)
			/ sizeof(simple_initiator_socket<SimpleBusAT> );
	assert(portId < nr_of_targets);
	payload_ptr->set_address(payload_ptr->get_address() | portId << 28);

	tlm_phase phase = BEGIN_RESP;
	sc_time t = get_transport_delay(payload_ptr->get_data_length());

	simple_target_socket_tagged < SimpleBusAT > *initiatorSocket =
			it->second.from;
// if BEGIN_RESP is send first we don't have to send END_REQ anymore
	it->second.from = 0;
// logging	cout << "\tBus: trans " << payload_ptr << "sent to initiator" << portId
// logging			<< ", phase: " << phase << endl;
	if(do_logging & LOG_BUS)
		cout << sc_time_stamp() << " " << name() << ": trans " << payload_ptr
				<< "sent to initiator" << portId << ", phase: " << phase
				<< endl;

	tlm_sync_enum sync = (*initiatorSocket)->nb_transport_bw(*payload_ptr,
			phase, t);
	switch(sync) {
		case TLM_COMPLETED:
			// Transaction finished
			mPendingTransactions.erase(payload_ptr);
			wait(t);
// logging		cout << "\tBus waiting " << t << "\n";
			if(do_logging & LOG_BUS)
				cout << sc_time_stamp() << " " << name() << ": waiting " << t
						<< "\n";

			break;

		case TLM_ACCEPTED:
		case TLM_UPDATED:
			// Transaction not yet finished
			// Error, 2-phase implementation shouldn't contain it
			cerr << sc_time_stamp() << " " << name()
					<< ":ERROR: Illegal phase received from initiator." << endl;
			assert(false);
			break;

		default:
			assert(0);
			exit(1);
	};
}

void SimpleBusAT::output_load() {
	cout << name() << " total transfer time  : " << total_transfer_time << endl;
	cout << name() << fixed << setprecision(1) << " load: transferring "
			<< (total_transfer_time) / (sc_time_stamp()) * 100 << "%." << endl;
}
