#include "TcpConnecting.hpp"

#include <faucet/tcp/TcpSocket.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>

using namespace boost::asio::ip;

TcpConnecting::TcpConnecting(TcpSocket &socket) :
	ConnectionState(socket), resolver(Asio::getIoService()), abortRequested(
			false), ipv6Attempted(false), host(), port() {
}

void TcpConnecting::enter(const char *host, uint16_t port) {
	this->host = host;
	this->port = boost::lexical_cast<std::string>(port);
	ipv6Attempted = false;
	startResolve(tcp::v4());
}

void TcpConnecting::abort() {
	resolver.cancel();
	abortRequested = true;
}

void TcpConnecting::startResolve(const protocol_type &protocol) {
	tcp::resolver::query query(protocol, host, port,
			tcp::resolver::query::numeric_service | tcp::resolver::query::address_configured);
	resolver.async_resolve(query, boost::bind(&TcpConnecting::handleResolve,
			this, socket->shared_from_this(), boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
}

void TcpConnecting::handleResolve(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());

	if (abortRequested) {
		return;
	}

	if (!error) {
		startConnectionAttempt(socket, endpointIterator);
	} else if(!ipv6Attempted) {
		startResolve(tcp::v6());
		ipv6Attempted = true;
	} else {
		enterErrorState(error.message());
	}
}

void TcpConnecting::startConnectionAttempt(boost::shared_ptr<TcpSocket> socket,
		tcp::resolver::iterator endpointIterator) {
	boost::system::error_code closeError;
	if (getSocket().close(closeError)) {
		Asio::getIoService().post(boost::bind(&TcpConnecting::handleConnect,
				this, socket, closeError, tcp::resolver::iterator()));
		return;
	}
	tcp::endpoint endpoint = *endpointIterator;
	getSocket().async_connect(endpoint, boost::bind(
			&TcpConnecting::handleConnect, this, socket,
			boost::asio::placeholders::error, ++endpointIterator));
}

void TcpConnecting::handleConnect(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());

	if (abortRequested) {
		return;
	}

	if (!error) {
		enterConnectedState();
	} else if (endpointIterator != tcp::resolver::iterator()) {
		startConnectionAttempt(socket, endpointIterator);
	} else if(!ipv6Attempted) {
		startResolve(tcp::v6());
		ipv6Attempted = true;
	} else {
		enterErrorState(error.message());
	}
}
