#pragma once

#include <faucet/Socket.hpp>

#include <cstddef>

/**
 * Common interface of all connection-oriented, stream-like sockets.
 *
 * The API functions which were historically named tcp_* / socket_* look up
 * their handle as StreamSocket, so that they work the same on a plain
 * TcpSocket and on sockets layered on top of one (e.g. TlsSocket).
 */
class StreamSocket: public Socket {
public:
	virtual ~StreamSocket() {}

	/**
	 * True while the connection is not yet ready for exchanging application data.
	 */
	virtual bool isConnecting() = 0;

	/**
	 * Commit everything written so far for sending.
	 */
	virtual void send() = 0;

	/**
	 * Try to receive the given number of bytes into the internal receive buffer.
	 *
	 * Returns true on success, false on failure.
	 * All data previously read into the internal buffer is discarded.
	 */
	virtual bool receive(size_t ammount) = 0;

	/**
	 * Read as much data as is available. Returns the number of bytes received.
	 * All data previously read into the internal buffer is discarded.
	 */
	virtual size_t receive() = 0;

	/**
	 * True if the connection has been closed in the receiving direction
	 * and all received data has been handed out.
	 */
	virtual bool isEof() = 0;

	virtual void disconnectAbortive() = 0;

	/**
	 * Enable / disable the TCP_NODELAY option (= disable / enable Nagle's algorithm)
	 */
	virtual bool setNoDelay(bool noDelay) = 0;
};
