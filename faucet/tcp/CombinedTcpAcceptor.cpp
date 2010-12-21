#include "CombinedTcpAcceptor.hpp"

using namespace boost::asio::ip;

CombinedTcpAcceptor::CombinedTcpAcceptor(uint16_t port) :
		v4Acceptor_(tcp::endpoint(tcp::v4(), port)),
		v6Acceptor_(tcp::endpoint(tcp::v6(), port)),
		checkV6First_(false) {
}

CombinedTcpAcceptor::~CombinedTcpAcceptor() {}

const std::string &CombinedTcpAcceptor::getErrorMessage() {
	static std::string message;
	message = "IPv4: "+v4Acceptor_.getErrorMessage()+"; IPv6: "+v6Acceptor_.getErrorMessage();
	return message;
}

bool CombinedTcpAcceptor::hasError() {
	return v4Acceptor_.hasError() && v6Acceptor_.hasError();
}

TcpSocket *CombinedTcpAcceptor::accept() {
	TcpSocket *acceptedSocket;
	if(checkV6First_) {
		acceptedSocket = v6Acceptor_.accept();
		if(acceptedSocket == NULL) {
			acceptedSocket = v4Acceptor_.accept();
		}
	} else {
		acceptedSocket = v4Acceptor_.accept();
		if(acceptedSocket == NULL) {
			acceptedSocket = v6Acceptor_.accept();
		}
	}

	checkV6First_ = !checkV6First_;
	return acceptedSocket;
}
