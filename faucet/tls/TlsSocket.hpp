#pragma once

#include <faucet/tcp/StreamSocket.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tls/SchannelSession.hpp>
#include <faucet/Buffer.hpp>

#include <boost/integer.hpp>
#include <boost/utility.hpp>
#include <memory>
#include <string>
#include <vector>

/**
 * A TLS client connection, which behaves just like a TcpSocket towards the
 * library user.
 *
 * This is a thin layer wrapped around an ordinary TcpSocket and a TLS session.
 * The TcpSocket does all the actual networking exactly as it does for plain
 * connections, it just happens to transport ciphertext. This class shovels
 * bytes between the two whenever one of its methods is called ("pumping").
 * There is no background activity of its own, so the connection only makes
 * progress while the library user keeps calling into the socket, e.g. by
 * checking for errors or received data once per step.
 *
 * Like the rest of the API facing side of the library, this class must only
 * be used from one thread at a time.
 */
class TlsSocket: public StreamSocket, boost::noncopyable {
public:
	virtual ~TlsSocket();

	/**
	 * Create a new socket representing a TLS connection to the given host
	 * and port. The host name is used for validating the server certificate.
	 */
	static std::shared_ptr<TlsSocket> connectTo(const char *host, uint16_t port);

	// Fallible
	virtual bool hasError();
	virtual std::string getErrorMessage();

	// ReadWritable
	virtual void write(const uint8_t *in, size_t size);
	virtual size_t read(uint8_t *out, size_t size);
	virtual std::string readString(size_t size);
	virtual size_t bytesRemaining() const;
	virtual void setReadpos(size_t pos);

	// Socket
	virtual size_t getSendbufferSize();
	virtual size_t getReceivebufferSize();
	virtual void setSendbufferLimit(size_t maxSize);
	virtual Buffer &getReceiveBuffer();
	virtual std::string getRemoteIp();
	virtual uint16_t getRemotePort();
	virtual uint16_t getLocalPort();

	// StreamSocket
	virtual bool isConnecting();
	virtual void send();
	virtual bool receive(size_t ammount);
	virtual size_t receive();
	virtual bool isEof();
	virtual void disconnectAbortive();
	virtual bool setNoDelay(bool noDelay);

private:
	TlsSocket(std::shared_ptr<TcpSocket> transport, const std::string &hostname);

	/** Carries the ciphertext */
	std::shared_ptr<TcpSocket> transport_;
	SchannelSession session_;

	/** Application data written by the user, but not committed with send() yet */
	std::vector<uint8_t> uncommitted_;
	/** Committed application data which could not be encrypted yet */
	std::vector<uint8_t> committed_;
	size_t sendbufferSizeLimit_;

	Buffer receiveBuffer_;

	bool handshakeDone_;
	bool closed_;
	bool failed_;
	/** Whether hasError() has returned true yet, see isEof() */
	bool errorReported_;
	std::string errorMessage_;

	/**
	 * Move data between the transport and the TLS session in both directions,
	 * as far as currently possible.
	 */
	void pump();
	void sendOutgoing();
	void fail(const std::string &message);
};
