#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>
using namespace boost::asio::ip;

TcpAcceptor::TcpAcceptor(const tcp::endpoint &endpoint) :
		acceptor_(*ioService),
		socket_(new tcp::socket(*ioService)),
		socketIsConnected_(false),
		hasError_(false),
		errorMessage_() {
	try {
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		acceptor_.async_accept(*socket_, boost::bind(
				&TcpAcceptor::handleAccept,
				this,
				boost::asio::placeholders::error));
	} catch(boost::system::system_error &newErr) {
		acceptor_.close();
		hasError_ = true;
		errorMessage_ = newErr.code().message();
	}
}

TcpAcceptor::~TcpAcceptor() {
	delete socket_;
}

const std::string &TcpAcceptor::getErrorMessage() {
	return errorMessage_;
}

bool TcpAcceptor::hasError() {
	return hasError_;
}

TcpSocket *TcpAcceptor::accept() {
	if(socketIsConnected_) {
		// Ownership of the socket transfers to the TcpSocket
		TcpSocket *tcpSocket = TcpSocket::fromConnectedSocket(socket_);
		socket_ = new tcp::socket(*ioService);
		socketIsConnected_ = false;
		acceptor_.async_accept(*socket_, boost::bind(
				&TcpAcceptor::handleAccept,
				this,
				boost::asio::placeholders::error));
		return tcpSocket;
	} else {
		return NULL;
	}
}

void TcpAcceptor::handleAccept(const boost::system::error_code &error) {
	if(!error) {
		socketIsConnected_ = true;
	} else {
		hasError_ = true;
		errorMessage_ = error.message();
	}
}
