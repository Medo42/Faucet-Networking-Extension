#pragma once

#include <faucet/Asio.hpp>

#include <boost/thread/recursive_mutex.hpp>
#include <string>

class TcpSocket;
class SendBuffer;

class ConnectionState {
public:
	/**
	 * Abort all running operations, or at least make sure that
	 * no further action is taken that would change the TcpSocket.
	 *
	 * This method is called before transitioning to error state and during an abortive close.
	 */
	virtual void abort(TcpSocket &socket) = 0;

	virtual bool isErrorState() {
		return false;
	}
	virtual std::string getErrorMessage() {
		return "";
	}
	virtual bool isConnecting() {
		return false;
	}

	virtual bool allowWrite() = 0;

	// Default for startAsyncSend: do nothing
	virtual void startAsyncSend(TcpSocket &socket);

	virtual bool isEof(TcpSocket &socket) = 0;
protected:
	/*
	 * Indirection functions to prevent having to make all states friends of TcpSocket
	 */
	void enterErrorState(TcpSocket &socket, const std::string &message);
	void enterConnectedState(TcpSocket &socket);
	boost::asio::ip::tcp::socket &getSocket(TcpSocket &socket);
	boost::recursive_mutex &getCommonMutex(TcpSocket &socket);
	SendBuffer &getSendBuffer(TcpSocket &socket);
};
