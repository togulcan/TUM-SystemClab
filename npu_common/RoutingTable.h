/**
 * @file	RoutingTable.h
 *
 * @date	Mar 11, 2011
 * @author	Miklos Kirilly
 */

#ifndef ROUTINGTABLE_H_
#define ROUTINGTABLE_H_

#include <vector>
#include <string>

/**
 * Lookup table for IP packet routing.
 * It does not implement any dynamic discovery mechanism, it works
 * with a static set of routing rules.
 *
 * It uses a configuration file which defines the routing table entries.
 * The lines of the table take the following form:
 @verbatim
 <network address> | <subnet mask> | <tx MAC port ID>
 @endverbatim
 * Lines beginning with '#' and empty lines are skipped. An example configuration
 * file may look like this:
 @verbatim
 # network address | subnet mask | next hop address (MAC port ID, 0..3)
 127.0.0.0 | 255.255.0.0 | 3
 192.168.0.1 | 255.255.255.255 | 0
 192.168.0.0 | 255.255.255.248 | 1
 192.168.0.11| 255.255.255.255 | 2
 192.168.0.0 | 255.255.255.0   | 3
 139.133.0.0 | 255.255.0.0     | 2
 0.0.0.0 | 0.0.0.0 | 3
 @endverbatim
 */
class RoutingTable {
public:
	/**
	 * Constructor to build the lookup table, reads the data from a file.
	 */
	RoutingTable(const char * fileName, char delimiter = '|');
	/**
	 * Returns the ID of the MAC that needs to be used for the next hop.
	 * @param destAddress destination address as an integer
	 */
	unsigned int getNextHop(unsigned int destAddress);

protected:
	/**
	 * This struct holds a simplified entry in the routing table.
	 * It does not contain information about the distance to the
	 * next routing target, because in the example application
	 * every destination is represented by the output MAC that
	 * has to be used and no further details of the routing are
	 * modelled.
	 */
	struct RoutingEntry {
		/// Subnet mask used for prefix extraction.
		unsigned int subnetMask;
		/// The prefix that the destination address
		/// has to match to be routed according to this entry.
		unsigned int netAddress;
		/// Originally the destination address, in
		/// this application the number of output port
		/// where the package is given out.
		unsigned int nextHop;
	};

	typedef std::vector<RoutingEntry> Container;
	typedef Container::iterator Iter;

	/**
	 * Reads the IP address from the string.
	 *
	 * @param str - the string containing the IPv4 address
	 * 				in numbers-and-dots format (e.g. "127.0.0.1").
	 * @note	The function may modify the parameter if it
	 * 			contains leading whitespace.
	 * @return the integer value of the address, 0 if the string is invalid.
	 */
	static unsigned int parseAddress(std::string& str);

	/**
	 * The STL container that holds the routing data.
	 */
	Container table;
};

#endif /* ROUTINGTABLE_H_ */
