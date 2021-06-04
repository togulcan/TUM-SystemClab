/**
 * @file	RoutingTable.cpp
 *
 * @date	Mar 11, 2011
 * @author	Miklos Kirilly
 */

#include "RoutingTable.h"
#include "TokenIterator.h"
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
// whitespace checking
#include <cctype>
// for converting string IP addresses to numbers
#include <arpa/inet.h>

using namespace std;

RoutingTable::RoutingTable(const char* fileName, char delimiter) {
	char lineBuffer[200];
	ifstream iFile(fileName);
#ifdef DEBUG
	cerr << "opened file " << fileName << endl;
#endif
	while(iFile.eof() == false) {
		// read one line
		iFile.getline(lineBuffer, sizeof(lineBuffer) / sizeof(char));

		// handle the line
		if(lineBuffer[0] == '#' || lineBuffer[0] == '\n'
				|| lineBuffer[0] == '\r' || lineBuffer[0] == '\0') {
			// lines that begin with '#' are treated as comment, empty lines are omitted
#ifdef DEBUG
			cerr << "line skipped" << endl;
#endif
		}
		else {
			// create Delimiter object from the single delimiter character and whitespace
			Delimiters delimiters(delimiter);
			// create a mutable pointer to the lineBuffer
			char* pLineBuffer = lineBuffer;
			// the words parsed will be packed into a vector
			vector < string > wordList;

			// create TokenIterators that can be used to tokenize the line
			TokenIterator<char*, Delimiters> charIter(pLineBuffer,
					pLineBuffer + strlen(pLineBuffer), delimiters), end2;
			// tokenize the line, resulting words are stored in wordList
			copy(charIter, end2, back_inserter(wordList));

			// make sure every parameter of a table row (RoutingEntry) can be initialized
			if(wordList.size() != 3) {
				// the line should contain exactly 3 parameters
				cerr << "format error in LUT initializer file" << endl
						<< "\tno of words: " << wordList.size() << endl;
			}
			else {
#ifdef DEBUG
				cerr << "from words: ";
				for (unsigned u = 0; u < 3; u++) {
					cerr << wordList.at(u) << ", " << endl;
				}
#endif
				RoutingEntry re;
				re.netAddress = RoutingTable::parseAddress(wordList.at(0));
				re.subnetMask = RoutingTable::parseAddress(wordList.at(1));
				re.nextHop = RoutingTable::parseAddress(wordList.at(2));
				table.push_back(re);
#ifdef DEBUG
				cerr << "read entry NA: " << re.netAddress << ", SM: "
				<< re.subnetMask << endl;
#endif
			}
		}
	}

}

unsigned int RoutingTable::getNextHop(unsigned int destAddress) {
	unsigned int longestMatchMask = 0;
	unsigned int longestMatchNextHop = 0;

	// analyze each entry
	for(Iter it = table.begin(); it != table.end(); it++) {
#ifdef DEBUG
		cout << "comparing " << (destAddress & it->subnetMask) << " <-> " << it->netAddress
		<< endl;
#endif
		// Address masked with subnet mask and checked if it is equals
		// the required network address.
		if(((destAddress & it->subnetMask) == it->netAddress)
				&& (it->subnetMask > longestMatchMask)) {

			// longest match, save it!
			longestMatchMask = it->subnetMask;
			longestMatchNextHop = it->nextHop;
		}
	}
	return longestMatchNextHop;
}

unsigned int RoutingTable::parseAddress(std::string& str) {
	/*
	 * The string contains an IPv4 address in the human
	 * readable numbers-and-dots notation. However, it might
	 * begin with whitespace, while the function inet_aton()
	 * (see below) expects the string to begin with the address.
	 * So whitespace has to be removed first.
	 */
	unsigned idx; // idx will hold the index in the string, where the address begins.
	// go through the string until we get to a number
	for(idx = 0; idx < str.length(); idx++) {
		if(isdigit(str.at(idx))) {
			break;
		}
	}
	// Erase leading whitespace
	str.erase(0, idx);

	// use standard function to read the address
	in_addr addr_struct;
	if(inet_aton(str.c_str(), &addr_struct)) {
		// convert to local number format (endianness)
		return ntohl(addr_struct.s_addr);
	}
	else {
		return 0;
	}
}
