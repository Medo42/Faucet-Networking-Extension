#pragma once

#include <faucet/Asio.hpp>

#include <boost/thread/recursive_mutex.hpp>
#include <string>

class TcpSocket;
class SendBuffer;
class Buffer;

/**
 * Some considerations on using these state objects:
 *
 * There are two ways that control can enter these objects: either by the TcpSocket
 * they belong to or by callback handlers. If the object is called from the TcpSocket,
 * the common mutex of that socket must already be locked. Completion handlers always
 * lock that mutex before accessing any state of either the TcpSocket or the state object.
 *
 * The reference to the TcpSocket is stored as simple pointer here because we always
 * know that the object exists: If the call comes from the TcpSocket this is directly
 * obvious, and the completion handlers always contain a shared pointer reference
 * to the socket to prevent it from being destroyed before all asynchronous requests are
 * completed. We could use a shared pointer reference instead, but that would prevent
 * the socket from being destroyed at all if the last external reference to it is
 * destroyed.
 */
class ConnectionState {
public:
	virtual ~ConnectionState() {}

	/**
	 * Abort all running operations, or at least make sure that
	 * no further action is taken that would change the TcpSocket.
	 *
	 * This method is called before transitioning to error state and during an abortive close.
	 */
	virtual void abort() = 0;

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
	virtual void startAsyncSend() {
	}
	virtual bool isEof() = 0;
	virtual bool receive(size_t ammount) {
		return false;
	}
	virtual void receive() {
		return;
	}
protected:
	TcpSocket *socket;

	ConnectionState(TcpSocket &tcpSocket);

	/*
	 * Indirection functions to prevent having to make all states friends of TcpSocket
	 */
	void enterErrorState(const std::string &message);
	void enterConnectedState();
	void setEndpointInfo(std::string remoteIp, uint16_t remotePort, uint16_t localPort);
	boost::asio::ip::tcp::socket &getSocket();
	boost::recursive_mutex &getCommonMutex();
	SendBuffer &getSendBuffer();
	Buffer &getReceiveBuffer();
};
