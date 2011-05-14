#include "ConnectionState.hpp"

#include <faucet/tcp/TcpSocket.hpp>

ConnectionState::ConnectionState(TcpSocket &tcpSocket) :
	socket(&tcpSocket) {
}

void ConnectionState::enterErrorState(const std::string &message) {
	socket->enterErrorState(message);
}

void ConnectionState::enterConnectedState() {
	socket->enterConnectedState();
}

void ConnectionState::setRemoteIp(std::string ip) {
	socket->remoteIp_ = ip;
}

boost::asio::ip::tcp::socket &ConnectionState::getSocket() {
	return *(socket->socket_);
}

boost::recursive_mutex &ConnectionState::getCommonMutex() {
	return socket->commonMutex_;
}

SendBuffer &ConnectionState::getSendBuffer() {
	return socket->sendbuffer_;
}

Buffer &ConnectionState::getReceiveBuffer() {
	return socket->receiveBuffer_;
}
