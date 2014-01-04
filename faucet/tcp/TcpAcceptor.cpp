#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>

TcpAcceptor::TcpAcceptor() :
        socket_(),
		acceptor_(),
		hasError_(false),
		errorMessage_(),
		socketMutex_(),
		errorMutex_() {
}

std::shared_ptr<TcpAcceptor> TcpAcceptor::listen(std::shared_ptr<tcp::acceptor> acceptor) {
    std::shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	result->acceptor_ = acceptor;
	result->startAsyncAccept();
	return result;
}

std::shared_ptr<TcpAcceptor> TcpAcceptor::error(std::string message) {
	std::shared_ptr<TcpAcceptor> result(new TcpAcceptor());
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

std::shared_ptr<TcpSocket> TcpAcceptor::accept() {
	boost::lock_guard<boost::recursive_mutex> guard(socketMutex_);
	if(socket_) {
		// Ownership of the socket transfers to the TcpSocket
		auto tcpSocket = TcpSocket::fromConnectedSocket(socket_);
		socket_.reset();
		startAsyncAccept();
		return tcpSocket;
	} else {
		return nullptr;
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
	auto socket = std::make_shared<tcp::socket>(Asio::getIoService());
	acceptor_->async_accept(*socket, boost::bind(
			&TcpAcceptor::handleAccept,
			shared_from_this(),
			boost::asio::placeholders::error,
			socket));
}

void TcpAcceptor::handleAccept(const boost::system::error_code &error, std::shared_ptr<tcp::socket> socket) {
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
