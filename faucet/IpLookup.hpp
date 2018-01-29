#pragma once

#include "Handled.hpp"
#include <faucet/Asio.hpp>

#include <boost/utility.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <string>
#include <memory>

#include <faucet/resolve.hpp>

class IpLookup: public Handled,
		boost::noncopyable {
public:
	static std::shared_ptr<IpLookup> lookup(const char *lookup);
	static std::shared_ptr<IpLookup> lookup(const char *lookup, fct_lookup_protocol protocol);

	bool ready();
	bool hasNext();
	std::string nextResult();

private:
	IpLookup();
	void handleResolve(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator endpointIterator);
	void loadNext();

	boost::recursive_mutex commonMutex_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::asio::ip::tcp::resolver::iterator result_;
	std::string next_;
	bool lookupComplete_;
};
