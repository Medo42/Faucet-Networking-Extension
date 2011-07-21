#pragma once

#include <faucet/Asio.hpp>

/**
 * Java-style iterator here, because the C++ style was too much typing for my taste.
 */
template<typename InternetProtocol>
class V4FirstIterator {
private:
	typedef boost::asio::ip::basic_resolver_iterator<InternetProtocol> iterator;
	iterator v4Iterator, v6Iterator;

public:
	V4FirstIterator() : v4Iterator(), v6Iterator() {}

	V4FirstIterator(iterator iter) : v4Iterator(iter), v6Iterator(iter) {
		while(v4Iterator != iterator() && v4Iterator->endpoint().protocol() != InternetProtocol::v4()) {
			++v4Iterator;
		}
		while(v6Iterator != iterator() && v6Iterator->endpoint().protocol() != InternetProtocol::v6()) {
			++v6Iterator;
		}
	}

	boost::asio::ip::basic_endpoint<InternetProtocol> next() {
		boost::asio::ip::basic_endpoint<InternetProtocol> endpoint;
		if(v4Iterator != iterator()) {
			endpoint = v4Iterator->endpoint();
			do {
				++v4Iterator;
			} while(v4Iterator != iterator() && v4Iterator->endpoint().protocol() != InternetProtocol::v4());
		} else if(v6Iterator != iterator()) {
			endpoint = v6Iterator->endpoint();
			do {
				++v6Iterator;
			} while(v6Iterator != iterator() && v6Iterator->endpoint().protocol() != InternetProtocol::v6());
		}
		return endpoint;
	}

	bool hasNext() const {
		return v4Iterator != iterator() || v6Iterator != iterator();
	}
};
