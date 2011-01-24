#include "TcpSocket.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <limits>
#include <windows.h>

using namespace boost::asio::ip;

TcpSocket::TcpSocket(State initialState) :
		socket_(new tcp::socket(Asio::getIoService())),
		resolver_(Asio::getIoService()),
		state_(initialState),
		errorMessage_(),
		sendbuffer_(),
		sendbufferSizeLimit_(std::numeric_limits<size_t>::max()),
		asyncSendInProgress_(false),
		partialReceiveBuffer_(),
		receiveBuffer_(),
		asyncReceiveInProgress_(false) {
}

TcpSocket::TcpSocket(tcp::socket *connectedSocket) :
		socket_(connectedSocket),
		resolver_(Asio::getIoService()),
		state_(TCPSOCK_CONNECTED),
		errorMessage_(),
		sendbuffer_(),
		sendbufferSizeLimit_(std::numeric_limits<size_t>::max()),
		asyncSendInProgress_(false),
		partialReceiveBuffer_(),
		receiveBuffer_(),
		asyncReceiveInProgress_(false) {
	disableNagle();
}

bool TcpSocket::isConnecting() {
	return state_ == TCPSOCK_CONNECTING;
}

bool TcpSocket::hasError() {
	return state_ == TCPSOCK_FAILED;
}

std::string TcpSocket::getErrorMessage() {
	return errorMessage_;
}

void TcpSocket::write(const uint8_t *buffer, size_t size) {
	if(state_ == TCPSOCK_CONNECTED || state_ == TCPSOCK_CONNECTING) {
		if(sendbuffer_.totalSize() + size > sendbufferSizeLimit_) {
			handleError("The send buffer size limit was exceeded.");
			return;
		} else {
			sendbuffer_.push(buffer, size);
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
	return sendbuffer_.totalSize();
}

void TcpSocket::setSendbufferLimit(size_t maxSize) {
	sendbufferSizeLimit_ = maxSize;
}

void TcpSocket::send() {
	if(state_ == TCPSOCK_CONNECTED || state_ == TCPSOCK_CONNECTING) {
		sendbuffer_.commit();
		if(state_ == TCPSOCK_CONNECTED) {
			startAsyncSend();
		}
	}
}

/**
 * @throws
 */
void TcpSocket::nonblockReceive(size_t maxData) {
	if(asyncReceiveInProgress_ || state_ != TCPSOCK_CONNECTED) {
		return;
	}

	size_t available = socket_->available();
	size_t readAmmount = std::min(maxData, available);
	size_t recvBufferEndIndex = partialReceiveBuffer_.size();
	partialReceiveBuffer_.insert(partialReceiveBuffer_.end(), readAmmount, 0);
	boost::asio::read(*socket_, boost::asio::buffer(partialReceiveBuffer_.data()+recvBufferEndIndex, readAmmount));
}

bool TcpSocket::receive(size_t ammount) {
	receiveBuffer_.clear();
	if(asyncReceiveInProgress_ || state_ != TCPSOCK_CONNECTED) {
		return false;
	}

	if(partialReceiveBuffer_.size() < ammount) {
		// Try to satisfy the request with a nonblocking read
		try {
			nonblockReceive(ammount-partialReceiveBuffer_.size());
		} catch(boost::system::system_error &e) {
			handleError(e.code().message());
			return false;
		}
	}

	if(partialReceiveBuffer_.size() >= ammount) {
		receiveBuffer_.write(partialReceiveBuffer_.data(), ammount);
		partialReceiveBuffer_.erase(partialReceiveBuffer_.begin(), partialReceiveBuffer_.begin()+ammount);
		return true;
	} else {
		size_t remaining = ammount - partialReceiveBuffer_.size();
		startAsyncReceive(remaining);
		return false;
	}
}

size_t TcpSocket::receive() {
	receiveBuffer_.clear();
	if(asyncReceiveInProgress_ || state_ != TCPSOCK_CONNECTED) {
		return 0;
	}

	try {
		nonblockReceive(std::numeric_limits<size_t>::max());
		receiveBuffer_.write(partialReceiveBuffer_.data(), partialReceiveBuffer_.size());
		partialReceiveBuffer_.clear();
	} catch(boost::system::system_error &e) {
		handleError(e.code().message());
	}
	return receiveBuffer_.size();
}

bool TcpSocket::isEof() {
	switch(state_) {
	case TCPSOCK_CONNECTING:
		return false;
	case TCPSOCK_CONNECTED:
		if(partialReceiveBuffer_.size()>0 || asyncReceiveInProgress_) {
			return false;
		}
		try {
			uint8_t nonsenseBuffer;
			boost::asio::socket_base::non_blocking_io command(true);
			socket_->io_control(command);
			boost::system::error_code error;
			socket_->receive(boost::asio::buffer(&nonsenseBuffer, 1), socket_->message_peek, error);
			command = false;
			socket_->io_control(command);
			if(error == boost::asio::error::eof) {
				return true;
			} else if(error == boost::asio::error::would_block) {
				return false;
			} else if(error) {
				throw boost::system::system_error(error);
			} else {
				return false;
			}
		} catch(boost::system::system_error &e) {
			handleError(e.code().message());
			return true;
		}
	default:
		return true;
	}
}

void TcpSocket::disconnect(bool hard) {
	if(state_ == TCPSOCK_CONNECTED || state_ == TCPSOCK_CONNECTING) {
		if(hard) {
			if(socket_->is_open()) {
				try {
					boost::asio::socket_base::linger option(false, 0);
					socket_->set_option(option);
					socket_->shutdown(tcp::socket::shutdown_both);
					socket_->close();
				} catch(boost::system::system_error &e) {
					handleError(e.code().message());
					return;
				}
			}
			state_ = TCPSOCK_CLOSED;
		} else {
			send();
			state_ = TCPSOCK_CLOSING;
		}
	}
}

boost::shared_ptr<TcpSocket> TcpSocket::connectTo(const char *address, uint16_t port) {
	boost::shared_ptr<TcpSocket> newSocket(new TcpSocket(TCPSOCK_CONNECTING));
	if(!newSocket->hasError()) {
		tcp::resolver::query query(address, boost::lexical_cast<std::string>(port), tcp::resolver::query::numeric_service);
		newSocket->resolver_.async_resolve(query, boost::bind(
				&TcpSocket::handleResolve,
				newSocket,
				boost::asio::placeholders::error,
				boost::asio::placeholders::iterator));
	}
	return newSocket;
}

boost::shared_ptr<TcpSocket> TcpSocket::error(const std::string &message) {
	boost::shared_ptr<TcpSocket> newSocket(new TcpSocket(TCPSOCK_FAILED));
	newSocket->errorMessage_ = message;
	return newSocket;
}

boost::shared_ptr<TcpSocket> TcpSocket::fromConnectedSocket(tcp::socket *connectedSocket) {
	boost::shared_ptr<TcpSocket> newSocket(new TcpSocket(connectedSocket));
	return newSocket;
}

void TcpSocket::handleError(const std::string &errorMessage) {
	// Don't overwrite error info, subsequent errors might
	// just be repercussions of the first one.
	if(state_ != TCPSOCK_FAILED) {
		state_ = TCPSOCK_FAILED;
		errorMessage_ = errorMessage;
	}

	// Try to close the socket (abortive), ignoring any new errors
	if(socket_->is_open()) {
		boost::system::error_code newError;
		boost::asio::socket_base::linger option(false, 0);
		socket_->set_option(option, newError);
		socket_->shutdown(tcp::socket::shutdown_both, newError);
		socket_->close(newError);
	}

	// We won't send or receive anything anymore
	// Clearing these should be safe, since closing the socket aborts all
	// asynchronous operations immediately
	sendbuffer_.clear();
	partialReceiveBuffer_.clear();
}

void TcpSocket::handleResolve(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(state_ == TCPSOCK_CONNECTING || state_ == TCPSOCK_CLOSING) {
		if(!error) {
			boost::system::error_code hostNotFound = boost::asio::error::host_not_found;
			handleConnect(hostNotFound, endpointIterator);
		} else {
			handleError(error.message());
		}
	}
}

void TcpSocket::handleConnect(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(state_ == TCPSOCK_CONNECTING || state_ == TCPSOCK_CLOSING) {
		if(!error) {
			disableNagle();
			if(!hasError()) {
				if(state_ == TCPSOCK_CONNECTING) {
					state_ = TCPSOCK_CONNECTED;
				}
				if(sendbuffer_.committedSize() > 0) {
					startAsyncSend();
				} else if(state_ == TCPSOCK_CLOSING) {
					boost::system::error_code closeError;
					if(socket_->close(closeError)) {
						handleError(closeError.message());
						return;
					}
					state_ = TCPSOCK_CLOSED;
				}
			}
		} else if(endpointIterator != tcp::resolver::iterator()) {
			boost::system::error_code closeError;
			if(socket_->close(closeError)) {
				handleError(closeError.message());
				return;
			}
			tcp::endpoint endpoint = *endpointIterator;
			socket_->async_connect(endpoint,
					boost::bind(&TcpSocket::handleConnect, shared_from_this(),
					boost::asio::placeholders::error, ++endpointIterator));
		} else {
			handleError(error.message());
		}
	}
}

void TcpSocket::startAsyncSend() {
	if(!asyncSendInProgress_) {
		asyncSendInProgress_ = true;
		socket_->async_send(sendbuffer_.committedAsConstBufferSequence(),
				boost::bind(
						&TcpSocket::handleSend,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
}

void TcpSocket::startAsyncReceive(size_t ammount) {
	if(!asyncReceiveInProgress_) {
		asyncReceiveInProgress_ = true;

		size_t recvBufferEndIndex = partialReceiveBuffer_.size();
		partialReceiveBuffer_.insert(partialReceiveBuffer_.end(), ammount, 0);
		boost::asio::async_read(*socket_, boost::asio::buffer(partialReceiveBuffer_.data()+recvBufferEndIndex, ammount),
				boost::bind(
						&TcpSocket::handleReceive,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
}

void TcpSocket::handleSend(const boost::system::error_code &error,
		size_t bytesTransferred) {
	asyncSendInProgress_ = false;
	if(state_ == TCPSOCK_CONNECTED || state_ == TCPSOCK_CLOSING) {
		if(!error) {
			sendbuffer_.pop(bytesTransferred);
			if(sendbuffer_.committedSize() > 0) {
				startAsyncSend();
			} else if(state_ == TCPSOCK_CLOSING) {
				boost::system::error_code closeError;
				if(socket_->close(closeError)) {
					handleError(closeError.message());
					return;
				}
				state_ = TCPSOCK_CLOSED;
			}
		} else {
			handleError(error.message());
		}
	}
}

void TcpSocket::handleReceive(const boost::system::error_code &error,
		size_t bytesTransferred) {
	asyncReceiveInProgress_ = false;
	if(error) {
		handleError(error.message());
	}
}

void TcpSocket::disableNagle() {
	try {
		tcp::no_delay nodelay(true);
		socket_->set_option(nodelay);
	} catch(boost::system::system_error &error) {
		handleError(error.code().message());
	}
}
