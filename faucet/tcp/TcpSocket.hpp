#pragma once
#include <faucet/Socket.h>
#include <faucet/Fallible.hpp>
#include <faucet/asio.h>
#include <faucet/SendBuffer.hpp>
#include <boost/integer.hpp>
#include <string>

using namespace boost::asio::ip;

class TcpSocket : public Socket {
public:
	virtual ~TcpSocket();

	virtual bool isConnecting();
	virtual const std::string &getErrorMessage();
	virtual bool hasError();

	void write(const uint8_t *buffer, size_t size);

	size_t getSendbufferSize();
	void setSendbufferLimit(size_t maxSize);

	void send();

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static TcpSocket *connectTo(const char *numericAddress, uint16_t port);

	/**
	 * Create an error socket with the given parameter as error message
	 */
	static TcpSocket *error(const std::string &message);

	/**
	 * Create a connected TcpSocket from an existing connected tcp::socket
	 */
	static TcpSocket *fromConnectedSocket(tcp::socket *connectedSocket);
private:
	enum State {
		TCPSOCK_CONNECTING,
		TCPSOCK_CONNECTED,
		TCPSOCK_CLOSED,
		TCPSOCK_FAILED
	};

	tcp::socket *socket_;
	tcp::resolver resolver_;

	State state_;
	std::string errorMessage_;

	SendBuffer sendbuffer_;
	size_t sendbufferSizeLimit_;

	bool asyncSendInProgress_;

	TcpSocket(State initialState);
	TcpSocket(tcp::socket *socket);

	void disableNagle();

	void handleError(const std::string &errorMessage);
	void handleResolve(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);
	void handleConnect(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);

	void startAsyncSend();
	void handleSend(const boost::system::error_code &err,
			size_t bytesTransferred);
};
