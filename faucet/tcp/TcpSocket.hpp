#pragma once

#include <faucet/Socket.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/Asio.hpp>
#include <faucet/tcp/SendBuffer.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/tcp/connectionStates/ConnectionState.hpp>
#include <faucet/tcp/connectionStates/TcpConnecting.hpp>
#include <faucet/tcp/connectionStates/TcpConnected.hpp>
#include <faucet/tcp/connectionStates/TcpClosed.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/utility.hpp>
#include <string>

class TcpSocket: public Socket,
		public ReadWritable,
		public boost::enable_shared_from_this<TcpSocket>,
		boost::noncopyable {
	friend class ConnectionState;

public:
	~TcpSocket();

	virtual bool isConnecting();
	virtual std::string getErrorMessage();
	virtual bool hasError();

	// Functions required by the ReadWritable interface
	virtual void write(const uint8_t *in, size_t size);
	virtual size_t read(uint8_t *out, size_t size);
	virtual std::string readString(size_t size);
	virtual size_t bytesRemaining() const;

	Buffer &getReceiveBuffer();

	size_t getSendbufferSize();
	void setSendbufferLimit(size_t maxSize);

	void send();
	/**
	 * Try to receive the given number of bytes into the internal receive buffer.
	 *
	 * Returns true on success, false on failure.
	 * All data previously read into the internal buffer is discarded.
	 */
	bool receive(size_t ammount);

	/**
	 * Read as much data as is available.
	 */
	size_t receive();

	/**
	 * Receive data until the delimiter or the end of the stream is found.
	 *
	 * Returns 0 if nothing was received, 1 if a delimited message was
	 * received and 2 if the remaining undelimited data at the end of the
	 * stream was received.
	 */
	int receiveDelimited(size_t maxSize);

	/**
	 * True if the connection has been closed in the receiving direction.
	 */
	bool isEof();

	void disconnectAbortive();

	std::string getRemoteIp();

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static boost::shared_ptr<TcpSocket> connectTo(const char *host,
			uint16_t port);

	/**
	 * Create an error socket with the given parameter as error message
	 */
	static boost::shared_ptr<TcpSocket> error(const std::string &message);

	/**
	 * Create a connected TcpSocket from an existing connected tcp::socket
	 */
	static boost::shared_ptr<TcpSocket> fromConnectedSocket(boost::shared_ptr<
			boost::asio::ip::tcp::socket> connectedSocket);

private:
	/**
	 * commonMutex_ is das big lock for TcpSocket and its related state objects.
	 * I tried more finely grained locking first, but it resulted in deadlock prone
	 * situations.
	 */
	boost::recursive_mutex commonMutex_;

	/*
	 * The following members are accessed from both the client thread and
	 * from completion handlers. For all access to them the common mutex
	 * must be locked first.
	 */
	boost::shared_ptr<boost::asio::ip::tcp::socket> socket_;
	TcpConnecting tcpConnecting_;
	TcpConnected tcpConnected_;
	TcpClosed tcpClosed_;
	ConnectionState *state_;

	SendBuffer sendbuffer_;
	std::string remoteIp_;

	/*
	 * The following members are only accessed from the client thread and
	 * don't need synchronization.
	 */
	std::string tokenDelimiter_;

	Buffer receiveBuffer_;
	size_t sendbufferSizeLimit_;

	TcpSocket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);

	void enterConnectingState(const char *host, uint16_t port);
	void enterConnectedState();
	void enterClosedState();
	void enterErrorState(const std::string &message);
};
