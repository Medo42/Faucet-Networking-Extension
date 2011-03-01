#include "CombinedTcpAcceptor.hpp"
#include <faucet/tcp/TcpAcceptor.hpp>

using namespace boost::asio::ip;

CombinedTcpAcceptor::CombinedTcpAcceptor(uint16_t port) :
		v4Acceptor_(TcpAcceptor::listen(tcp::endpoint(tcp::v4(), port))),
		v6Acceptor_(TcpAcceptor::listen(tcp::endpoint(tcp::v6(), port))),
		checkV6First_(false) {
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

boost::shared_ptr<TcpSocket> CombinedTcpAcceptor::accept() {
	boost::shared_ptr<TcpSocket> acceptedSocket;
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
