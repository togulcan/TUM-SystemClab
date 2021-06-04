/**
 * @file	TokenIterator.h
 * This header contains utility functions for tokenizing the config file
 * of the RoutingTable.
 *
 * The content of this file is from Bruce Eckel's book <emph>Thinking in C++, 2nd edition</emph>.
 *
 * @date	Mar 25, 2011
 * @author
 */

#ifndef TOKENITERATOR_H_
#define TOKENITERATOR_H_

#include <string>
#include <iterator>
#include <algorithm>
#include <cctype>

struct Isalpha {
	bool operator()(char c) {
		using namespace std;
		//[[For a compiler bug]]
		return isalpha(c);
	}
};

/**
 *
 */
class Delimiters {
	std::string exclude;
public:
	Delimiters() {
	}
	Delimiters(const std::string& excl) :
			exclude(excl) {
	}
	Delimiters(char excl) :
			exclude(1, excl) {
	}
	bool operator()(char c) {
		return exclude.find(c) == std::string::npos;
	}
};

/**
 * @ingroup utility
 */
template<class InputIter, class Pred = Isalpha>
class TokenIterator: public std::iterator<std::input_iterator_tag, std::string,
		std::ptrdiff_t> {
	InputIter first;
	InputIter last;
	std::string word;
	Pred predicate;
public:
	TokenIterator(InputIter begin, InputIter end, Pred pred = Pred()) :
			first(begin), last(end), predicate(pred) {
		++*this;
	}
	TokenIterator() {
	} // End sentinel
	// Prefix increment:
	TokenIterator& operator++() {
		word.resize(0);
		first = std::find_if(first, last, predicate);
		while(first != last && predicate(*first))
			word += *first++;
		return *this;
	}
	// Postfix increment
	class Proxy {
		std::string word;
	public:
		Proxy(const std::string& w) :
				word(w) {
		}
		std::string operator*() {
			return word;
		}
	};
	Proxy operator++(int) {
		Proxy d(word);
		++*this;
		return d;
	}
	// Produce the actual value:
	std::string operator*() const {
		return word;
	}
	std::string* operator->() const {
		return &(operator*());
	}
	// Compare iterators:
	bool operator==(const TokenIterator&) {
		return word.size() == 0 && first == last;
	}
	bool operator!=(const TokenIterator& rv) {
		return !(*this == rv);
	}
};
/// @endcond
#endif /* TOKENITERATOR_H_ */
