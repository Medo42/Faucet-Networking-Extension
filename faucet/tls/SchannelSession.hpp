#pragma once

#include <boost/integer.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/**
 * A TLS client session implemented on top of SChannel, the TLS implementation
 * that ships with Windows.
 *
 * This class deliberately knows nothing about sockets. It is a pure state
 * machine: ciphertext received from the peer is passed in through feed(),
 * ciphertext that has to be sent to the peer piles up in outgoing(), and
 * decrypted application data piles up in incoming(). The owner is responsible
 * for moving the bytes between this object and the actual transport.
 *
 * The server certificate is validated against the Windows certificate store,
 * including a check that it was issued for the host name passed to the
 * constructor. That host name is also sent to the server as SNI.
 *
 * Not thread safe.
 */
class SchannelSession {
public:
	explicit SchannelSession(const std::string &hostname);
	~SchannelSession();

	/**
	 * Pass in ciphertext that was received from the peer and process as much
	 * of it as possible. This may add data to both outgoing() and incoming().
	 */
	void feed(const uint8_t *data, size_t size);

	/**
	 * Ciphertext that needs to be sent to the peer. The caller should send
	 * the entire content and then clear() the vector.
	 */
	std::vector<uint8_t> &outgoing();

	/**
	 * Decrypted application data received from the peer. The caller should
	 * remove whatever it consumes.
	 */
	std::vector<uint8_t> &incoming();

	/**
	 * Encrypt application data and append the resulting ciphertext to outgoing().
	 *
	 * This is only possible while isEstablished() is true, otherwise nothing
	 * happens and false is returned.
	 */
	bool encrypt(const uint8_t *data, size_t size);

	/**
	 * Add a TLS close notification to outgoing(), if the session got far
	 * enough for that to make sense. No application data can be sent afterwards.
	 */
	void shutdown();

	/**
	 * True while a handshake is in progress. Application data can not be
	 * encrypted during this time.
	 */
	bool isHandshaking() const;

	/**
	 * True if the handshake is complete and application data can be exchanged.
	 */
	bool isEstablished() const;

	/**
	 * True if the peer has sent a TLS close notification, which means that
	 * no more application data will arrive.
	 */
	bool peerClosed() const;

	bool hasError() const;
	const std::string &getErrorMessage() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

	SchannelSession(const SchannelSession &);
	SchannelSession &operator=(const SchannelSession &);
};
