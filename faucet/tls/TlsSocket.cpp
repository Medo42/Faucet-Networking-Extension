#include "TlsSocket.hpp"

#include <limits>

TlsSocket::TlsSocket(std::shared_ptr<TcpSocket> transport, const std::string &hostname) :
		transport_(transport), session_(hostname), uncommitted_(), committed_(),
		sendbufferSizeLimit_(std::numeric_limits<size_t>::max()), receiveBuffer_(),
		handshakeDone_(false), closed_(false), failed_(false), errorReported_(false),
		errorMessage_() {
}

TlsSocket::~TlsSocket() {
	/*
	 * Try to end the session politely. Everything handed to the transport
	 * here is still sent after this object is gone, because the TcpSocket
	 * lives on until its pending send operations are complete.
	 *
	 * Note that application data which could not be encrypted yet because
	 * the handshake is still running is lost at this point.
	 */
	if (!failed_ && !closed_) {
		pump();
	}
	if (!failed_ && !closed_) {
		session_.shutdown();
		sendOutgoing();
	}
}

std::shared_ptr<TlsSocket> TlsSocket::connectTo(const char *host, uint16_t port) {
	std::shared_ptr<TlsSocket> result(new TlsSocket(TcpSocket::connectTo(host, port), host));
	// The session has already produced its first handshake message.
	// The transport will send it as soon as the connection is up.
	result->pump();
	return result;
}

void TlsSocket::fail(const std::string &message) {
	if (!failed_) {
		failed_ = true;
		errorMessage_ = message;
		transport_->disconnectAbortive();
	}
}

void TlsSocket::sendOutgoing() {
	std::vector<uint8_t> &outgoing = session_.outgoing();
	if (!outgoing.empty()) {
		transport_->write(outgoing.data(), outgoing.size());
		transport_->send();
		outgoing.clear();
	}
}

void TlsSocket::pump() {
	if (failed_ || closed_) {
		return;
	}

	if (transport_->hasError()) {
		fail(transport_->getErrorMessage());
		return;
	}

	// Ciphertext from the peer
	size_t received = transport_->receive();
	if (received > 0) {
		session_.feed(transport_->getReceiveBuffer().getData(), received);
	}

	if (session_.isEstablished()) {
		handshakeDone_ = true;
		// Application data from the user
		if (!committed_.empty()) {
			session_.encrypt(committed_.data(), committed_.size());
			committed_.clear();
		}
	}

	// Ciphertext to the peer
	sendOutgoing();

	if (session_.hasError()) {
		fail(session_.getErrorMessage());
	} else if (!handshakeDone_ && transport_->isEof()) {
		fail("The connection was closed during the TLS handshake.");
	}
}

bool TlsSocket::hasError() {
	pump();
	if (failed_) {
		errorReported_ = true;
	}
	return failed_;
}

std::string TlsSocket::getErrorMessage() {
	return errorMessage_;
}

void TlsSocket::write(const uint8_t *in, size_t size) {
	if (failed_ || closed_) {
		return;
	}
	if (getSendbufferSize() + size > sendbufferSizeLimit_) {
		fail("The send buffer size limit was exceeded.");
		return;
	}
	uncommitted_.insert(uncommitted_.end(), in, in + size);
}

size_t TlsSocket::read(uint8_t *out, size_t size) {
	return receiveBuffer_.read(out, size);
}

std::string TlsSocket::readString(size_t size) {
	return receiveBuffer_.readString(size);
}

size_t TlsSocket::bytesRemaining() const {
	return receiveBuffer_.bytesRemaining();
}

void TlsSocket::setReadpos(size_t pos) {
	receiveBuffer_.setReadpos(pos);
}

size_t TlsSocket::getSendbufferSize() {
	// Mixes plaintext and ciphertext sizes, which is close enough for a limit
	return uncommitted_.size() + committed_.size() + transport_->getSendbufferSize();
}

size_t TlsSocket::getReceivebufferSize() {
	return receiveBuffer_.size();
}

void TlsSocket::setSendbufferLimit(size_t maxSize) {
	sendbufferSizeLimit_ = maxSize;
}

Buffer &TlsSocket::getReceiveBuffer() {
	return receiveBuffer_;
}

std::string TlsSocket::getRemoteIp() {
	return transport_->getRemoteIp();
}

uint16_t TlsSocket::getRemotePort() {
	return transport_->getRemotePort();
}

uint16_t TlsSocket::getLocalPort() {
	return transport_->getLocalPort();
}

bool TlsSocket::isConnecting() {
	pump();
	return !failed_ && !closed_ && !handshakeDone_;
}

void TlsSocket::send() {
	committed_.insert(committed_.end(), uncommitted_.begin(), uncommitted_.end());
	uncommitted_.clear();
	pump();
}

bool TlsSocket::receive(size_t ammount) {
	receiveBuffer_.clear();
	pump();
	if (failed_ || closed_) {
		return false;
	}

	std::vector<uint8_t> &incoming = session_.incoming();
	if (incoming.size() < ammount) {
		return false;
	}
	receiveBuffer_.write(incoming.data(), ammount);
	incoming.erase(incoming.begin(), incoming.begin() + ammount);
	return true;
}

size_t TlsSocket::receive() {
	receiveBuffer_.clear();
	pump();
	if (failed_ || closed_) {
		return 0;
	}

	std::vector<uint8_t> &incoming = session_.incoming();
	receiveBuffer_.write(incoming.data(), incoming.size());
	incoming.clear();
	return receiveBuffer_.size();
}

bool TlsSocket::isEof() {
	pump();
	if (failed_) {
		/*
		 * Any call into this socket can be the one that runs into an error,
		 * including the pump() above. A typical user checks for errors once
		 * at the start of each step and for EOF further down. If we reported
		 * EOF right away, such a user would take a failed connection for a
		 * normally ended one, e.g. accept a download that was cut short by
		 * a decryption error, or report "unexpected end of stream" instead
		 * of the actual reason like a rejected certificate. So we hold back
		 * the EOF until the error has been seen.
		 */
		return errorReported_;
	}
	if (closed_) {
		return true;
	}
	if (!handshakeDone_ || !session_.incoming().empty()) {
		return false;
	}
	/*
	 * A proper TLS shutdown is signalled by the peer with a close notification,
	 * but plenty of servers just close the TCP connection instead. We report
	 * both as a normal end of the stream. This means a connection cut off by
	 * an attacker can not be told apart from one the peer ended, so protocols
	 * on top of this need their own way of knowing that a message is
	 * complete (like the Content-Length header in HTTP).
	 *
	 * pump() has just moved everything that was readable from the transport
	 * through the session, so if the transport reports EOF now there really
	 * is nothing left to wait for.
	 */
	return session_.peerClosed() || transport_->isEof();
}

void TlsSocket::disconnectAbortive() {
	closed_ = true;
	transport_->disconnectAbortive();
}

bool TlsSocket::setNoDelay(bool noDelay) {
	return transport_->setNoDelay(noDelay);
}
