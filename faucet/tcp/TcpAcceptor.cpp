#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>

TcpAcceptor::TcpAcceptor() :
		acceptor_(),
		socket_(),
		hasError_(false),
		errorMessage_(),
		socketMutex_(),
		errorMutex_() {
}

shared_ptr<TcpAcceptor> TcpAcceptor::listen(boost::shared_ptr<tcp::acceptor> acceptor) {
	shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	result->acceptor_ = acceptor;
	result->startAsyncAccept();
	return result;
}

shared_ptr<TcpAcceptor> TcpAcceptor::error(std::string message) {
	shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	result->hasError_ = true;
	result->errorMessage_ = message;
	return result;
}

TcpAcceptor::~TcpAcceptor() {}

std::string TcpAcceptor::getErrorMessage() {
	boost::lock_guard<boost::recursive_mutex> guard(errorMutex_);
	return errorMessage_;
}

bool TcpAcceptor::hasError() {
	boost::lock_guard<boost::recursive_mutex> guard(errorMutex_);
	return hasError_;
}

shared_ptr<TcpSocket> TcpAcceptor::accept() {
	boost::lock_guard<boost::recursive_mutex> guard(socketMutex_);
	if(socket_) {
		// Ownership of the socket transfers to the TcpSocket
		shared_ptr<TcpSocket> tcpSocket = TcpSocket::fromConnectedSocket(socket_);
		socket_.reset();
		startAsyncAccept();
		return tcpSocket;
	} else {
		return boost::shared_ptr<TcpSocket>();
	}
}

void TcpAcceptor::close() {
	boost::lock_guard<boost::recursive_mutex> guard(socketMutex_);
	boost::system::error_code error;
	if(acceptor_) {
		acceptor_->close(error);
	}
}

void TcpAcceptor::startAsyncAccept() {
	shared_ptr<tcp::socket> socket(new tcp::socket(Asio::getIoService()));
	acceptor_->async_accept(*socket, boost::bind(
			&TcpAcceptor::handleAccept,
			shared_from_this(),
			boost::asio::placeholders::error,
			socket));
}

void TcpAcceptor::handleAccept(const boost::system::error_code &error, shared_ptr<tcp::socket> socket) {
	boost::lock_guard<boost::recursive_mutex> guard(socketMutex_);
	if(!error) {
		socket_ = socket;
	} else {
		if(acceptor_->is_open()) {
			startAsyncAccept();
		} else {
			boost::lock_guard<boost::recursive_mutex> guard(errorMutex_);
			hasError_ = true;
			errorMessage_ = error.message();
		}
	}
}
