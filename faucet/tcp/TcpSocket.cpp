#include "TcpSocket.hpp"

#include <boost/thread/locks.hpp>

bool TcpSocket::isConnecting() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return state_->isConnecting();
}

std::string TcpSocket::getErrorMessage() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return state_->getErrorMessage();
}

bool TcpSocket::hasError() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return state_->isErrorState();
}

void TcpSocket::write(const uint8_t *in, size_t size) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	if(state_->allowWrite()) {
		if(sendbuffer_.totalSize() + size > sendbufferSizeLimit_) {
			enterErrorState("The send buffer size limit was exceeded.");
			return;
		} else {
			sendbuffer_.push(in, size);
		}
	}
}

size_t TcpSocket::read(uint8_t *out, size_t size) {
	return receiveBuffer_.read(out, size);
}

size_t TcpSocket::bytesRemaining() const {
	return receiveBuffer_.bytesRemaining();
}

Buffer &TcpSocket::getReceiveBuffer() {
	return receiveBuffer_;
}

size_t TcpSocket::getSendbufferSize() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return sendbuffer_.totalSize();
}

void TcpSocket::setSendbufferLimit(size_t maxSize) {
	sendbufferSizeLimit_ = maxSize;
}

void TcpSocket::send() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	sendbuffer_.commit();
	state_->startAsyncSend(*this);
}

bool receive(size_t ammount);
size_t receive();

bool TcpSocket::isEof() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return state_->isEof(*this);
}

void disconnect(bool hard);

void TcpSocket::enterErrorState(const std::string &message) {
	state_->abort(*this);
	state_ = &tcpError_;
	tcpError_.enter(*this, message);
}

void TcpSocket::enterConnectedState() {
	state_ = &tcpConnected_;
}
