#pragma once
#include <faucet/Socket.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/Asio.hpp>
#include <faucet/tcp/SendBuffer.hpp>
#include <faucet/Buffer.hpp>
#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

using namespace boost::asio::ip;

class TcpSocket : public Socket, public ReadWritable, public boost::enable_shared_from_this<TcpSocket> {
public:
	virtual bool isConnecting();
	virtual std::string getErrorMessage();
	virtual bool hasError();

	// Functions required by the ReadWritable interface
	virtual void write(const uint8_t *in, size_t size);
	virtual size_t read(uint8_t *out, size_t size);
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

	std::vector<uint8_t> partialReceiveBuffer_;
	Buffer receiveBuffer_;
	bool asyncReceiveInProgress_;

	TcpSocket(State initialState);
	TcpSocket(tcp::socket *socket);

	void disableNagle();
	void nonblockReceive(size_t maxData);
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
