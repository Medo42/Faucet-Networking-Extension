#include "TcpAcceptor.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/bind.hpp>
using namespace boost::asio::ip;

TcpAcceptor::TcpAcceptor() :
		acceptor_(Asio::getIoService()),
		socket_(0),
		hasError_(false),
		errorMessage_(),
		socketMutex_(),
		errorMutex_() {
}

boost::shared_ptr<TcpAcceptor> TcpAcceptor::listen(const tcp::endpoint &endpoint) {
	boost::shared_ptr<TcpAcceptor> result(new TcpAcceptor());
	try {
		result->acceptor_.open(endpoint.protocol());
		result->acceptor_.set_option(tcp::acceptor::reuse_address(true));
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

TcpAcceptor::~TcpAcceptor() {
	boost::lock_guard<boost::mutex> guard(socketMutex_);
	delete socket_;
}

std::string TcpAcceptor::getErrorMessage() {
	boost::lock_guard<boost::mutex> guard(errorMutex_);
	return errorMessage_;
}

bool TcpAcceptor::hasError() {
	boost::lock_guard<boost::mutex> guard(errorMutex_);
	return hasError_;
}

boost::shared_ptr<TcpSocket> TcpAcceptor::accept() {
	boost::lock_guard<boost::mutex> guard(socketMutex_);
	if(socket_) {
		// Ownership of the socket transfers to the TcpSocket
		boost::shared_ptr<TcpSocket> tcpSocket = TcpSocket::fromConnectedSocket(socket_);
		socket_ = 0;
		startAsyncAccept();
		return tcpSocket;
	} else {
		return boost::shared_ptr<TcpSocket>();
	}
}

void TcpAcceptor::startAsyncAccept() {
	tcp::socket *socket = new tcp::socket(Asio::getIoService());
	acceptor_.async_accept(*socket, boost::bind(
			&TcpAcceptor::handleAccept,
			shared_from_this(),
			boost::asio::placeholders::error,
			socket));
}

void TcpAcceptor::handleAccept(const boost::system::error_code &error, tcp::socket *socket) {
	if(!error) {
		boost::lock_guard<boost::mutex> guard(socketMutex_);
		socket_ = socket;
	} else {
		delete socket;
		boost::lock_guard<boost::mutex> guard(errorMutex_);
		hasError_ = true;
		errorMessage_ = error.message();
	}
}
