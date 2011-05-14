#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>

TcpAcceptor::TcpAcceptor() :
		acceptor_(Asio::getIoService()),
		socket_(),
		hasError_(false),
		errorMessage_(),
		socketMutex_(),
		errorMutex_() {
}

shared_ptr<TcpAcceptor> TcpAcceptor::listen(const tcp::endpoint &endpoint) {
	shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	try {
		result->acceptor_.open(endpoint.protocol());
		result->acceptor_.set_option(tcp::acceptor::reuse_address(true));
		if(endpoint.protocol() == tcp::v6()) {
			result->acceptor_.set_option(v6_only(true));
		}
		result->acceptor_.bind(endpoint);
		result->acceptor_.listen();

	} catch(boost::system::system_error &newErr) {
		boost::system::error_code error;
		result->acceptor_.close(error);
		result->hasError_ = true;
		result->errorMessage_ = newErr.code().message();
		return result;
	}

	result->startAsyncAccept();
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
	boost::system::error_code error;
	acceptor_.close(error);
}

void TcpAcceptor::startAsyncAccept() {
	shared_ptr<tcp::socket> socket(new tcp::socket(Asio::getIoService()));
	acceptor_.async_accept(*socket, boost::bind(
			&TcpAcceptor::handleAccept,
			shared_from_this(),
			boost::asio::placeholders::error,
			socket));
}

void TcpAcceptor::handleAccept(const boost::system::error_code &error, shared_ptr<tcp::socket> socket) {
	if(!error) {
		boost::lock_guard<boost::recursive_mutex> guard(socketMutex_);
		socket_ = socket;
	} else {
		boost::lock_guard<boost::recursive_mutex> guard(errorMutex_);
		hasError_ = true;
		errorMessage_ = error.message();
	}
}
