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

#include <boost/integer.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/utility.hpp>
#include <string>
#include <memory>

class TcpSocket: public Socket,
		public std::enable_shared_from_this<TcpSocket>,
		boost::noncopyable {
	friend class ConnectionState;

public:
	virtual ~TcpSocket();

	virtual bool isConnecting();
	virtual std::string getErrorMessage();
	virtual bool hasError();

	// Functions required by the ReadWritable interface
	virtual void write(const uint8_t *in, size_t size);
	virtual size_t read(uint8_t *out, size_t size);
	virtual std::string readString(size_t size);
	virtual size_t bytesRemaining() const;
	virtual void setReadpos(size_t pos);

	virtual size_t getSendbufferSize();
	virtual size_t getReceivebufferSize();
	virtual void setSendbufferLimit(size_t maxSize);

	virtual Buffer &getReceiveBuffer();
	virtual std::string getRemoteIp();
	virtual uint16_t getRemotePort();
	virtual uint16_t getLocalPort();

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
	 * True if the connection has been closed in the receiving direction.
	 */
	bool isEof();

	void disconnectAbortive();

    /**
     * Enable / disable the TCP_NODELAY option (= disable / enable Nagle's algorithm)
     */
    bool setNoDelay(bool noDelay);

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static std::shared_ptr<TcpSocket> connectTo(const char *host,
			uint16_t port);

	/**
	 * Create an error socket with the given parameter as error message
	 */
	static std::shared_ptr<TcpSocket> error(const std::string &message);

	/**
	 * Create a connected TcpSocket from an existing connected tcp::socket
	 */
	static std::shared_ptr<TcpSocket> fromConnectedSocket(std::shared_ptr<
			boost::asio::ip::tcp::socket> connectedSocket);

    static const bool DEFAULT_TCP_NODELAY = true;

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
	std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
	TcpConnecting tcpConnecting_;
	TcpConnected tcpConnected_;
	TcpClosed tcpClosed_;
	ConnectionState *state_;

	SendBuffer sendbuffer_;
	std::string remoteIp_;
	uint16_t remotePort_;
	uint16_t localPort_;

	/*
	 * The following members are only accessed from the client thread and
	 * don't need synchronization.
	 */
	Buffer receiveBuffer_;
	size_t sendbufferSizeLimit_;

	TcpSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

	void enterConnectingState(const char *host, uint16_t port);
	void enterConnectedState(bool noDelay);
	void enterClosedState();
	void enterErrorState(const std::string &message);
};
