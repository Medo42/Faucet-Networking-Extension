#include "TcpSocket.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::asio::ip;

TcpSocket::TcpSocket(State initialState) :
		socket_(new tcp::socket(*ioService)),
		resolver_(*ioService),
		state_(initialState),
		errorMessage_() {
}

TcpSocket::TcpSocket(State initialState, tcp::socket *socket) :
		socket_(socket),
		resolver_(*ioService),
		state_(initialState),
		errorMessage_() {
}

TcpSocket::~TcpSocket() {
	delete socket_;
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
	tcp::resolver::query query(address, boost::lexical_cast<std::string>(port), tcp::resolver::query::numeric_service);
	newSocket->resolver_.async_resolve(query, boost::bind(
			&TcpSocket::handleResolve,
			newSocket,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator));
	return newSocket;
}

TcpSocket *TcpSocket::error(const std::string &message) {
	TcpSocket *newSocket = new TcpSocket(TCPSOCK_FAILED);
	newSocket->errorMessage_ = message;
	return newSocket;
}

TcpSocket *TcpSocket::fromConnectedSocket(tcp::socket *connectedSocket) {
	return new TcpSocket(TCPSOCK_CONNECTED, connectedSocket);
}

void TcpSocket::handleError(const boost::system::error_code &error) {
	// Don't overwrite error info, subsequent errors might
	// just be repercussions of the first one.
	if(state_ != TCPSOCK_FAILED) {
		state_ = TCPSOCK_FAILED;
		errorMessage_ = error.message();
	}
}

void TcpSocket::handleResolve(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		boost::system::error_code hostNotFound = boost::asio::error::host_not_found;
		handleConnect(hostNotFound, endpointIterator);
	} else {
		handleError(error);
	}
}

void TcpSocket::handleConnect(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		state_ = TCPSOCK_CONNECTED;
		// TODO attempt to send data that's already in the buffer
	} else if(endpointIterator != tcp::resolver::iterator()) {
		boost::system::error_code closeError;
		if(socket_->close(closeError)) {
			handleError(closeError);
		}
		tcp::endpoint endpoint = *endpointIterator;
		socket_->async_connect(endpoint,
				boost::bind(&TcpSocket::handleConnect, this,
				boost::asio::placeholders::error, ++endpointIterator));
	} else {
		handleError(error);
	}
}
