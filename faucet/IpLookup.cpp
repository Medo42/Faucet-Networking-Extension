#include "IpLookup.hpp"

#include <boost/thread/locks.hpp>
#include <boost/bind.hpp>
#include <memory>

using namespace boost::asio::ip;
std::shared_ptr<IpLookup> IpLookup::lookup(const char *lookup) {
	std::shared_ptr<IpLookup> ipLookup (new IpLookup());
	tcp::resolver::query query(lookup, "", tcp::resolver::query::address_configured);
	ipLookup->resolver_.async_resolve(query, boost::bind(&IpLookup::handleResolve,
			ipLookup, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	return ipLookup;
}

std::shared_ptr<IpLookup> IpLookup::lookup(const char *lookup, const tcp::resolver::protocol_type &protocol) {
	std::shared_ptr<IpLookup> ipLookup (new IpLookup());
	tcp::resolver::query query(protocol, lookup, "", tcp::resolver::query::address_configured);
	ipLookup->resolver_.async_resolve(query, boost::bind(&IpLookup::handleResolve,
			ipLookup, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	return ipLookup;
}

IpLookup::IpLookup() : commonMutex_(), resolver_(Asio::getIoService()), result_(), next_(), lookupComplete_(false) {}

void IpLookup::handleResolve(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	lookupComplete_ = true;
	if (!error) {
		result_ = endpointIterator;
		loadNext();
	}
}

bool IpLookup::ready() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return lookupComplete_;
}

bool IpLookup::hasNext() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return !next_.empty();
}

std::string IpLookup::nextResult() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	std::string ip = next_;
	loadNext();
	return ip;
}

void IpLookup::loadNext() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	if (result_ != tcp::resolver::iterator()) {
		boost::system::error_code ec;
		std::string ip = result_->endpoint().address().to_string(ec);
		if(ec) {
			result_ = tcp::resolver::iterator();
			next_ = "";
		} else {
			++result_;
			next_ = ip;
		}
	} else {
		next_ = "";
	}
}
