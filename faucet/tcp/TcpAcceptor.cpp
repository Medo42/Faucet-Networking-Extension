#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>
using namespace boost::asio::ip;

TcpAcceptor::TcpAcceptor() :
		acceptor_(Asio::getIoService()),
		socket_(new tcp::socket(Asio::getIoService())),
		socketIsConnected_(false),
		hasError_(false),
		errorMessage_() {
}

boost::shared_ptr<TcpAcceptor> TcpAcceptor::listen(const tcp::endpoint &endpoint) {
	boost::shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	try {
		result->acceptor_.open(endpoint.protocol());
		result->acceptor_.set_option(tcp::acceptor::reuse_address(true));
		result->acceptor_.bind(endpoint);
		result->acceptor_.listen();

		result->acceptor_.async_accept(*(result->socket_), boost::bind(
				&TcpAcceptor::handleAccept,
				result,
				boost::asio::placeholders::error));
	} catch(boost::system::system_error &newErr) {
		boost::system::error_code error;
		result->acceptor_.close(error);
		result->hasError_ = true;
		result->errorMessage_ = newErr.code().message();
	}
	return result;
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

boost::shared_ptr<TcpSocket> TcpAcceptor::accept() {
	if(socketIsConnected_) {
		// Ownership of the socket transfers to the TcpSocket
		boost::shared_ptr<TcpSocket> tcpSocket = TcpSocket::fromConnectedSocket(socket_);
		socket_ = new tcp::socket(Asio::getIoService());
		socketIsConnected_ = false;
		acceptor_.async_accept(*socket_, boost::bind(
				&TcpAcceptor::handleAccept,
				shared_from_this(),
				boost::asio::placeholders::error));
		return tcpSocket;
	} else {
		return boost::shared_ptr<TcpSocket>();
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
