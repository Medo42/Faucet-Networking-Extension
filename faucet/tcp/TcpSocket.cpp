#include "TcpSocket.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <limits>

using namespace boost::asio::ip;

TcpSocket::TcpSocket(State initialState) :
		socket_(new tcp::socket(Asio::getIoService())),
		resolver_(Asio::getIoService()),
		state_(initialState),
		sendbufferSizeLimit_(std::numeric_limits<size_t>::max()) {
}

TcpSocket::TcpSocket(tcp::socket *connectedSocket) :
		socket_(connectedSocket),
		resolver_(Asio::getIoService()),
		state_(TCPSOCK_CONNECTED),
		sendbufferSizeLimit_(std::numeric_limits<size_t>::max()) {
	disableNagle();
}

TcpSocket::~TcpSocket() {
	delete socket_;
}

bool TcpSocket::isConnecting() {
	return state_ == TCPSOCK_CONNECTING;
}

bool TcpSocket::hasError() {
	return state_ == TCPSOCK_FAILED;
}

const std::string &TcpSocket::getErrorMessage() {
	return errorMessage_;
}

void TcpSocket::write(const uint8_t *buffer, size_t size) {
	if(!hasError()) {
		if(sendbuffer_.totalSize() + size > sendbufferSizeLimit_) {
			handleError("The send buffer size limit was exceeded.");
		} else {
			sendbuffer_.push(buffer, size);
		}
	}
}

size_t TcpSocket::getSendbufferSize() {
	return sendbuffer_.totalSize();
}

void TcpSocket::setSendbufferLimit(size_t maxSize) {
	sendbufferSizeLimit_ = maxSize;
}

void TcpSocket::send() {
	sendbuffer_.commit();
	if(state_ == TCPSOCK_CONNECTED) {
		startAsyncSend();
	}
}

Buffer *TcpSocket::receive(size_t ammount) {
	if(asyncReceiveInProgress_ || hasError()) {
		return 0;
	}

	if(receiveBuffer_.size() < ammount) {
		// Try to satisfy the request with a nonblocking read
		try {
			size_t remaining = ammount - receiveBuffer_.size();
			size_t available = socket_->available();
			if(available >= remaining) {
					size_t recvBufferEndIndex = receiveBuffer_.size();
					receiveBuffer_.insert(receiveBuffer_.end(), remaining, 0);
					boost::asio::read(*socket_, boost::asio::buffer(receiveBuffer_.data()+recvBufferEndIndex, remaining));
			}
		} catch(boost::system::system_error &e) {
			handleError(e.code().message());
		}
	}

	if(receiveBuffer_.size() >= ammount) {
		Buffer *result = new Buffer();
		result->write(receiveBuffer_.data(), ammount);
		receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin()+ammount);
		return result;
	} else {
		// TODO: Start async read
		return 0;
	}
}

TcpSocket *TcpSocket::connectTo(const char *address, uint16_t port) {
	TcpSocket *newSocket = new TcpSocket(TCPSOCK_CONNECTING);
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

TcpSocket *TcpSocket::error(const std::string &message) {
	TcpSocket *newSocket = new TcpSocket(TCPSOCK_FAILED);
	newSocket->errorMessage_ = message;
	return newSocket;
}

TcpSocket *TcpSocket::fromConnectedSocket(tcp::socket *connectedSocket) {
	return new TcpSocket(connectedSocket);
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
	sendbuffer_.clear();
	receiveBuffer_.clear();
}

void TcpSocket::handleResolve(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		boost::system::error_code hostNotFound = boost::asio::error::host_not_found;
		handleConnect(hostNotFound, endpointIterator);
	} else {
		handleError(error.message());
	}
}

void TcpSocket::handleConnect(const boost::system::error_code &error,
		tcp::resolver::iterator endpointIterator) {
	if(!error) {
		disableNagle();
		if(!hasError()) {
			state_ = TCPSOCK_CONNECTED;
			if(sendbuffer_.committedSize() > 0) {
				startAsyncSend();
			}
		}
	} else if(endpointIterator != tcp::resolver::iterator()) {
		boost::system::error_code closeError;
		if(socket_->close(closeError)) {
			handleError(closeError.message());
		}
		tcp::endpoint endpoint = *endpointIterator;
		socket_->async_connect(endpoint,
				boost::bind(&TcpSocket::handleConnect, this,
				boost::asio::placeholders::error, ++endpointIterator));
	} else {
		handleError(error.message());
	}
}

void TcpSocket::handleSend(const boost::system::error_code &error,
		size_t bytesTransferred) {
	asyncSendInProgress_ = false;
	if(!error) {
		sendbuffer_.pop(bytesTransferred);
		if(sendbuffer_.committedSize() > 0) {
			startAsyncSend();
		}
	} else {
		handleError(error.message());
	}
}
void TcpSocket::startAsyncSend() {
	if(!asyncSendInProgress_) {
		asyncSendInProgress_ = true;
		socket_->async_write_some(sendbuffer_.committedAsConstBufferSequence(),
				boost::bind(
						&TcpSocket::handleSend,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
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

