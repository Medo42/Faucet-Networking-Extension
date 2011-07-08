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

void ConnectionState::setEndpointInfo(std::string remoteIp, uint16_t remotePort, uint16_t localPort) {
	socket->remoteIp_ = remoteIp;
	socket->remotePort_ = remotePort;
	socket->localPort_ = localPort;
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
