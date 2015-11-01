#include "CombinedTcpAcceptor.hpp"
#include <faucet/tcp/TcpAcceptor.hpp>

using namespace boost::asio::ip;

CombinedTcpAcceptor::CombinedTcpAcceptor(uint16_t port) :
		v4Acceptor_(),
		v6Acceptor_(),
		checkV6First_(false),
		localPort_(port) {

	auto v4acceptor = std::make_shared<tcp::acceptor>(Asio::getIoService());
	auto v6acceptor = std::make_shared<tcp::acceptor>(Asio::getIoService());
	boost::system::error_code ignoredError, v4Error, v6Error;

	if (localPort_ == 0) {
		try {
			tcp::acceptor dualstack_placeholder(Asio::getIoService());
			dualstack_placeholder.open(tcp::v6());
			dualstack_placeholder.set_option(v6_only(false));
			dualstack_placeholder.bind(tcp::endpoint(tcp::v6(), localPort_));
			dualstack_placeholder.listen();
			localPort_ = dualstack_placeholder.local_endpoint().port();
			dualstack_placeholder.close(ignoredError);
		} catch (boost::system::system_error &e) {
			// Error -> Probably no dual stack support or no v6 support at all
		}
	}

	try {
		v4acceptor->open(tcp::v4());
		v4acceptor->bind(tcp::endpoint(tcp::v4(), localPort_));
		v4acceptor->listen();
		localPort_ = v4acceptor->local_endpoint().port();
	} catch (boost::system::system_error &e) {
		// Error -> IPv4 port is probably in use
		v4Error = e.code();
		v4acceptor->close(ignoredError);
	}

	try {
		v6acceptor->open(tcp::v6());
		v6acceptor->set_option(v6_only(true), ignoredError);
		v6acceptor->bind(tcp::endpoint(tcp::v6(), localPort_));
		v6acceptor->listen();
		localPort_ = v6acceptor->local_endpoint().port();
	} catch (boost::system::system_error &e) {
		v6Error = e.code();
		v6acceptor->close(ignoredError);
	}

	if(v4acceptor->is_open()) {
		v4Acceptor_ = TcpAcceptor::listen(v4acceptor);
	} else {
		v4Acceptor_ = TcpAcceptor::error(v4Error.message());
	}

	if(v6acceptor->is_open()) {
		v6Acceptor_ = TcpAcceptor::listen(v6acceptor);
	} else {
		v6Acceptor_ = TcpAcceptor::error(v6Error.message());
	}
}

CombinedTcpAcceptor::~CombinedTcpAcceptor() {
	v4Acceptor_->close();
	v6Acceptor_->close();
}

std::string CombinedTcpAcceptor::getErrorMessage() {
	return "IPv4: "+v4Acceptor_->getErrorMessage()+"; IPv6: "+v6Acceptor_->getErrorMessage();
}

bool CombinedTcpAcceptor::hasError() {
	return v4Acceptor_->hasError() && v6Acceptor_->hasError();
}

std::shared_ptr<TcpSocket> CombinedTcpAcceptor::accept() {
	std::shared_ptr<TcpSocket> acceptedSocket;
	if(checkV6First_) {
		acceptedSocket = v6Acceptor_->accept();
		if(!acceptedSocket) {
			acceptedSocket = v4Acceptor_->accept();
		}
	} else {
		acceptedSocket = v4Acceptor_->accept();
		if(!acceptedSocket) {
			acceptedSocket = v6Acceptor_->accept();
		}
	}

	checkV6First_ = !checkV6First_;
	return acceptedSocket;
}

uint16_t CombinedTcpAcceptor::getLocalPort() {
	return localPort_;
}

bool CombinedTcpAcceptor::isListeningV4() {
	return !v4Acceptor_->hasError();
}

bool CombinedTcpAcceptor::isListeningV6() {
	return !v6Acceptor_->hasError();
}
