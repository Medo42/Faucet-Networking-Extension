#include "TcpConnecting.hpp"

#include <faucet/tcp/TcpSocket.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>

using namespace boost::asio::ip;

TcpConnecting::TcpConnecting(TcpSocket &socket) :
	ConnectionState(socket), resolver(Asio::getIoService()), abortRequested(
			false) {
}

void TcpConnecting::enter(const char *host, uint16_t port) {
	tcp::resolver::query query(host, boost::lexical_cast<std::string>(port),
			tcp::resolver::query::numeric_service | tcp::resolver::query::address_configured);
	resolver.async_resolve(query, boost::bind(&TcpConnecting::handleResolve,
			this, socket->shared_from_this(), boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
}

void TcpConnecting::abort() {
	resolver.cancel();
	abortRequested = true;
}

void TcpConnecting::handleResolve(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());

	if (abortRequested) {
		return;
	}

	if (error) {
		enterErrorState(error.message());
		return;
	}

	V4FirstIterator<tcp> endpoints(endpointIterator);
	boost::system::error_code ec;
	if(startConnectionAttempt(socket, endpoints, ec)) {
		enterErrorState(ec.message());
		return;
	}
}

boost::system::error_code TcpConnecting::startConnectionAttempt(boost::shared_ptr<TcpSocket> socket,
		V4FirstIterator<tcp> endpoints, boost::system::error_code &ec) {
	if (getSocket().close(ec)) {
		return ec;
	}
	tcp::endpoint endpoint = endpoints.next();
	getSocket().async_connect(endpoint, boost::bind(
			&TcpConnecting::handleConnect, this, socket,
			boost::asio::placeholders::error, endpoints));
	return ec;
}

void TcpConnecting::handleConnect(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		V4FirstIterator<tcp> endpoints) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());

	if (abortRequested) {
		return;
	}

	if (!error) {
		enterConnectedState();
	} else if (endpoints.hasNext()) {
		boost::system::error_code ec;
		if(startConnectionAttempt(socket, endpoints, ec)) {
			enterErrorState(ec.message());
			return;
		}
	} else {
		enterErrorState(error.message());
		return;
	}
}
