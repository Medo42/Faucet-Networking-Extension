#include "TcpSocket.hpp"

#include <boost/thread/locks.hpp>
#include <limits>

using namespace boost::asio::ip;

TcpSocket::~TcpSocket() {
	// Ensure graceful close
	if (socket_ && socket_->is_open()) {
		boost::system::error_code newError;
		socket_->shutdown(tcp::socket::shutdown_both, newError);
		socket_->close();
	}
}

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
	if (state_->allowWrite()) {
		if (sendbuffer_.totalSize() + size > sendbufferSizeLimit_) {
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

std::string TcpSocket::readString(size_t size) {
	return receiveBuffer_.readString(size);
}

size_t TcpSocket::bytesRemaining() const {
	return receiveBuffer_.bytesRemaining();
}

void TcpSocket::setReadpos(size_t pos) {
	receiveBuffer_.setReadpos(pos);
}

Buffer &TcpSocket::getReceiveBuffer() {
	return receiveBuffer_;
}

size_t TcpSocket::getSendbufferSize() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return sendbuffer_.totalSize();
}

size_t TcpSocket::getReceivebufferSize() {
	return receiveBuffer_.size();
}

void TcpSocket::setSendbufferLimit(size_t maxSize) {
	sendbufferSizeLimit_ = maxSize;
}

void TcpSocket::send() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	sendbuffer_.commit();
	state_->startAsyncSend();
}

bool TcpSocket::receive(size_t ammount) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	receiveBuffer_.clear();
	return state_->receive(ammount);
}

size_t TcpSocket::receive() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	receiveBuffer_.clear();
	state_->receive();
	return receiveBuffer_.size();
}

bool TcpSocket::isEof() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return state_->isEof();
}

void TcpSocket::disconnectAbortive() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	enterClosedState();
}

std::string TcpSocket::getRemoteIp() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return remoteIp_;
}

uint16_t TcpSocket::getRemotePort() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return remotePort_;
}

uint16_t TcpSocket::getLocalPort() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return localPort_;
}

std::shared_ptr<TcpSocket> TcpSocket::connectTo(const char *host,
		uint16_t port) {
    auto asioSocket = std::make_shared<tcp::socket>(Asio::getIoService());
	std::shared_ptr<TcpSocket> result(new TcpSocket(asioSocket));
	boost::lock_guard<boost::recursive_mutex> guard(result->commonMutex_);
	result->state_ = &(result->tcpConnecting_);
	result->tcpConnecting_.enter(host, port);
	return result;
}

std::shared_ptr<TcpSocket> TcpSocket::error(const std::string &message) {
    auto asioSocket = std::make_shared<tcp::socket>(Asio::getIoService());
    std::shared_ptr<TcpSocket> result(new TcpSocket(asioSocket));
	boost::lock_guard<boost::recursive_mutex> guard(result->commonMutex_);
	result->state_ = &(result->tcpClosed_);
	result->tcpClosed_.enterError(message);
	return result;
}

std::shared_ptr<TcpSocket> TcpSocket::fromConnectedSocket(
		std::shared_ptr<tcp::socket> connectedSocket) {
	std::shared_ptr<TcpSocket> result(new TcpSocket(connectedSocket));
	boost::lock_guard<boost::recursive_mutex> guard(result->commonMutex_);
	result->state_ = &(result->tcpConnected_);
	result->tcpConnected_.enter();
	return result;
}

TcpSocket::TcpSocket(std::shared_ptr<tcp::socket> socket) :
		commonMutex_(), socket_(socket), tcpConnecting_(*this), tcpConnected_(
				*this), tcpClosed_(*this), state_(0), sendbuffer_(), remoteIp_(), remotePort_(
				0), localPort_(0), receiveBuffer_(), sendbufferSizeLimit_(
				std::numeric_limits<size_t>::max()) {
}

void TcpSocket::enterConnectedState() {
	state_ = &tcpConnected_;
	tcpConnected_.enter();
}

void TcpSocket::enterErrorState(const std::string &message) {
	state_->abort();
	state_ = &tcpClosed_;
	tcpClosed_.enterError(message);
}

void TcpSocket::enterClosedState() {
	state_->abort();
	state_ = &tcpClosed_;
	tcpClosed_.enterClosed();
}
