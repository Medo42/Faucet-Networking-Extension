#include "IpLookup.hpp"

#include <boost/make_shared.hpp>
#include <boost/thread/locks.hpp>
#include <boost/bind.hpp>

using namespace boost::asio::ip;
boost::shared_ptr<IpLookup> IpLookup::lookup(const char *lookup) {
	boost::shared_ptr<IpLookup> ipLookup(new IpLookup());
	tcp::resolver::query query(lookup, "", tcp::resolver::query::address_configured);
	ipLookup->resolver_.async_resolve(query, boost::bind(&IpLookup::handleResolve,
			ipLookup, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	return ipLookup;
}

boost::shared_ptr<IpLookup> IpLookup::lookup(const char *lookup, const tcp::resolver::protocol_type &protocol) {
	boost::shared_ptr<IpLookup> ipLookup(new IpLookup());
	tcp::resolver::query query(protocol, lookup, "", tcp::resolver::query::address_configured);
	ipLookup->resolver_.async_resolve(query, boost::bind(&IpLookup::handleResolve,
			ipLookup, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	return ipLookup;
}

IpLookup::IpLookup() : commonMutex_(), resolver_(Asio::getIoService()), result_(), lookupComplete_(false) {}

void IpLookup::handleResolve(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	lookupComplete_ = true;
	if (!error) {
		result_ = endpointIterator;
	}
}

bool IpLookup::ready() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return lookupComplete_;
}

std::string IpLookup::nextResult() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	if (result_ == tcp::resolver::iterator()) {
		return "";
	} else {
		boost::system::error_code ec;
		std::string ip = result_->endpoint().address().to_string(ec);
		if(ec) {
			result_ = tcp::resolver::iterator();
			return "";
		}
		++result_;
		return ip;
	}
}
