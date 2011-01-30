#include "ConnectionState.hpp"

#include <faucet/tcp/TcpSocket.hpp>

void ConnectionState::startAsyncSend(TcpSocket &socket) {
}

void ConnectionState::enterErrorState(TcpSocket &socket, const std::string &message) {
	socket.enterErrorState(message);
}

void ConnectionState::enterConnectedState(TcpSocket &socket) {
	socket.enterConnectedState();
}

boost::asio::ip::tcp::socket &ConnectionState::getSocket(TcpSocket &socket) {
	return *socket.socket_;
}

boost::recursive_mutex &ConnectionState::getCommonMutex(TcpSocket &socket) {
	return socket.commonMutex_;
}

SendBuffer &ConnectionState::getSendBuffer(TcpSocket &socket) {
	return socket.sendbuffer_;
}
