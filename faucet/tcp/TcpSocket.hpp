#pragma once
#include <faucet/Socket.hpp>
#include <faucet/Writable.hpp>
#include <faucet/Asio.hpp>
#include <faucet/tcp/SendBuffer.hpp>
#include <faucet/Buffer.hpp>
#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

using namespace boost::asio::ip;

class TcpSocket : public Socket, public Writable, public boost::enable_shared_from_this<TcpSocket> {
public:
	virtual bool isConnecting();
	virtual const std::string &getErrorMessage();
	virtual bool hasError();

	void write(const uint8_t *buffer, size_t size);

	size_t getSendbufferSize();
	void setSendbufferLimit(size_t maxSize);

	void send();
	/**
	 * Try to receive the given number of bytes.
	 *
	 * Returns either a buffer with the requested ammount of data,
	 * or null if the request can't be immediately fulfilled.
	 */
	Buffer *receive(size_t ammount);

	/**
	 * Read as much data as is available.
	 */
	Buffer *receive();

	/**
	 * True if the connection has been closed in the receiving direction.
	 */
	bool isEof();

	void disconnect(bool hard);

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static boost::shared_ptr<TcpSocket> connectTo(const char *numericAddress, uint16_t port);

	/**
	 * Create an error socket with the given parameter as error message
	 */
	static boost::shared_ptr<TcpSocket> error(const std::string &message);

	/**
	 * Create a connected TcpSocket from an existing connected tcp::socket
	 */
	static boost::shared_ptr<TcpSocket> fromConnectedSocket(tcp::socket *connectedSocket);
private:
	enum State {
		TCPSOCK_CONNECTING,
		TCPSOCK_CONNECTED,
		TCPSOCK_CLOSING,
		TCPSOCK_CLOSED,
		TCPSOCK_FAILED
	};

	boost::scoped_ptr<tcp::socket> socket_;
	tcp::resolver resolver_;

	State state_;
	std::string errorMessage_;

	SendBuffer sendbuffer_;
	size_t sendbufferSizeLimit_;
	bool asyncSendInProgress_;

	std::vector<uint8_t> receiveBuffer_;
	bool asyncReceiveInProgress_;

	TcpSocket(State initialState);
	TcpSocket(tcp::socket *socket);

	void disableNagle();
	void nonblockReceiveAvailable();
	Buffer *bufferFromReceiveBuffer();

	void handleError(const std::string &errorMessage);
	void handleResolve(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);
	void handleConnect(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);

	void startAsyncSend();
	void startAsyncReceive(size_t ammount);

	void handleSend(const boost::system::error_code &err,
			size_t bytesTransferred);
	void handleReceive(const boost::system::error_code &err,
			size_t bytesTransferred);
};
