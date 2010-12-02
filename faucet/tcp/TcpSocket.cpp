#include "TcpSocket.h"
#include <faucet/tcp/TcpState.h>
#include <faucet/tcp/TcpConnecting.h>
#include <faucet/tcp/TcpClosed.h>

#include <boost/bind.hpp>

using namespace boost::asio::ip;

TcpSocket::TcpSocket(State initialState) :
		socket_(ioService),
		resolver_(ioService),
		state_(initialState),
		errorMessage_() {
}

TcpSocket::~TcpSocket() {
}

bool TcpSocket::isConnecting() {
	return state_ == TCPSOCK_CONNECTING;
}

bool TcpSocket::hasError() {
	return state_ == TCPSOCK_FAILED;
}

const std::string &TcpSocket::getErrorMessage() {
	return errorMessage_;
}

TcpSocket *TcpSocket::connectTo(const char *address, uint16_t port) {
	TcpSocket *newSocket = new TcpSocket(TCPSOCK_CONNECTING);
	tcp::resolver::query query(address, "");
	newSocket->resolver_.async_resolve(query, boost::bind(
			&TcpSocket::handleResolve,
			newSocket,
			port,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
	return newSocket;
}

TcpSocket *TcpSocket::error(const std::string &message) {
	TcpSocket *newSocket = new TcpSocket(TCPSOCK_FAILED);
	newSocket->errorMessage_ = message;
	return newSocket;
}

void TcpSocket::handleError(const boost::system::error_code &error) {
	// Don't overwrite error info, subsequent errors might
	// just be repercussions of the first one.
	if(state_ != TCPSOCK_FAILED) {
		state_ = TCPSOCK_FAILED;
		errorMessage_ = error.message();
	}
}

void TcpSocket::handleResolve(uint16_t port, const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		boost::system::error_code hostNotFound = boost::asio::error::host_not_found;
		handleConnect(port, hostNotFound, endpointIterator);
	} else {
		handleError(error);
	}
}

void TcpSocket::handleConnect(uint16_t port, const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		state_ = TCPSOCK_CONNECTED;
		// TODO attempt to send data that's already in the buffer
	} else if(endpointIterator != tcp::resolver::iterator()) {
		socket_.close();
		tcp::endpoint endpoint = *endpointIterator;
		endpoint.port(port);
		socket_.async_connect(endpoint,
				boost::bind(&TcpSocket::handleConnect, this, port,
				boost::asio::placeholders::error, ++endpointIterator));
	} else {
		handleError(error);
	}
}
