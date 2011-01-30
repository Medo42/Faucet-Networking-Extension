#include "TcpConnecting.hpp"

#include <faucet/tcp/TcpSocket.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>

using namespace boost::asio::ip;

TcpConnecting::TcpConnecting() :
	resolver(Asio::getIoService()), abortRequested(false) {
}

void TcpConnecting::enter(TcpSocket &socket, const char *host, uint16_t port) {
	tcp::resolver::query query(host, boost::lexical_cast<std::string>(port),
			tcp::resolver::query::numeric_service);
	resolver.async_resolve(query, boost::bind(&TcpConnecting::handleResolve,
			this, socket.shared_from_this(), boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
}

void TcpConnecting::abort(TcpSocket &socket) {
	resolver.cancel();
	abortRequested = true;
}

bool TcpError::isEof(TcpSocket &socket) {
	return false;
}

void TcpConnecting::handleResolve(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex(*socket));

	if (abortRequested)
		return;

	if (!error) {
		startConnectionAttempt(socket, endpointIterator);
	} else {
		enterErrorState(*socket, error.message());
	}
}

void TcpConnecting::startConnectionAttempt(boost::shared_ptr<TcpSocket> socket,
		tcp::resolver::iterator endpointIterator) {
	boost::system::error_code closeError;
	if (getSocket(*socket).close(closeError)) {
		Asio::getIoService().post(boost::bind(&TcpConnecting::handleConnect,
				this, socket, closeError, tcp::resolver::iterator()));
		return;
	}
	tcp::endpoint endpoint = *endpointIterator;
	getSocket(*socket).async_connect(endpoint, boost::bind(
			&TcpConnecting::handleConnect, this, socket,
			boost::asio::placeholders::error, ++endpointIterator));
}

void TcpConnecting::handleConnect(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex(*socket));

	if (abortRequested)
		return;

	if (!error) {
		enterConnectedState(*socket);
	} else if (endpointIterator != tcp::resolver::iterator()) {
		startConnectionAttempt(socket, endpointIterator);
	} else {
		enterErrorState(*socket, error.message());
	}
}
