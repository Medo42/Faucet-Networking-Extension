#include "UdpSocket.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>

using namespace boost::asio::ip;

UdpSocket::UdpSocket() :
		commonMutex_(), sendqueue_(), receivequeue_(), asyncSendInProgress_(
				false), ipv4socket_(Asio::getIoService()), ipv6socket_(
				Asio::getIoService()), resolver_(Asio::getIoService()), hasError_(false), errorMessage_(), localPort_(
				0), remoteIp_(), remotePort_(), receiveBuffer_(new Buffer()), sendBuffer_(
				new Buffer()) {
}

UdpSocket::~UdpSocket() {
}

/*
 * This code is intended to work on both WinXP and Vista/7, and allow binding to a random
 * unused port. Unfortunately there is no "safe" way to bind the same unused port for IPv4
 * and IPv6 in WinXP, and unfortunately binding a port with a Vista dual stack socket will
 * succeed even if the port is in use for IPv4 already, which would leave you with an IPv6
 * only socket which is not useful for most people at the moment.
 *
 * To work around these problems, I'm making some assumptions: First, binding a dual stack
 * socket to an unused port will actually bind to a port that is unused for both IPv4 and
 * IPv6, and not just for IPv6. Second, "displacing" the v4 part of a dual stack socket by
 * binding an explicit IPv4 socket to the same port is a well-behaved operation. Based on
 * this, here is how we bind:
 *
 * If the port number is not 0, we simply try binding an IPv4 and IPv6 socket to the given
 * port. If it is 0 and we are on Vista/7, we bind a dual stack socket first and steal its
 * IPv4 part for a seperate socket afterwards, which should always work if the assumptions
 * above hold. On WinXP with port 0, the dual stack socket creation will fail, and we just
 * attempt to open an IPv4 socket with an unused port, and then an IPv6 socket on the same
 * port.
 *
 * As a consequence, this operation is not reliable on WinXP, since the IPv6 part may fail
 * even though there might be free ports available.
 */
boost::shared_ptr<UdpSocket> UdpSocket::bind(uint16_t portnr) {
	boost::shared_ptr<UdpSocket> socketPtr(new UdpSocket());
	boost::system::error_code ignoredError, v4Error, v6Error;

	if (portnr == 0) {
		try {
			socketPtr->ipv6socket_.open(udp::v6());
			socketPtr->ipv6socket_.set_option(v6_only(false));
			socketPtr->ipv6socket_.bind(udp::endpoint(udp::v6(), portnr));
			portnr = socketPtr->ipv6socket_.local_endpoint().port();
		} catch (boost::system::system_error &e) {
			// Error -> Probably no dual stack support or no v6 support at all
			socketPtr->ipv6socket_.close(ignoredError);
		}
	}

	try {
		socketPtr->ipv4socket_.open(udp::v4());
		socketPtr->ipv4socket_.bind(udp::endpoint(udp::v4(), portnr));
		portnr = socketPtr->ipv4socket_.local_endpoint().port();
	} catch (boost::system::system_error &e) {
		// Error -> IPv4 port is probably in use
		v4Error = e.code();
		socketPtr->ipv4socket_.close(ignoredError);
	}

	if (!socketPtr->ipv6socket_.is_open()) {
		// We didn't create a dual stack socket earlier, try simple v6
		try {
			socketPtr->ipv6socket_.open(udp::v6());
			socketPtr->ipv6socket_.set_option(v6_only(true), ignoredError);
			socketPtr->ipv6socket_.bind(udp::endpoint(udp::v6(), portnr));
			portnr = socketPtr->ipv6socket_.local_endpoint().port();
		} catch (boost::system::system_error &e) {
			v6Error = e.code();
			socketPtr->ipv6socket_.close(ignoredError);
		}
	}

	if (!socketPtr->ipv4socket_.is_open()
			&& !socketPtr->ipv6socket_.is_open()) {
		socketPtr->hasError_ = true;
		socketPtr->errorMessage_ = "IPv4: " + v4Error.message() + ", IPv6:"
				+ v6Error.message();
	} else {
		socketPtr->localPort_ = portnr;
		if (socketPtr->ipv4socket_.is_open()) {
			boost::shared_array<uint8_t> buf(new uint8_t[65536]);
			socketPtr->asyncReceive(&(socketPtr->ipv4socket_), buf);
		}
		if (socketPtr->ipv6socket_.is_open()) {
			boost::shared_array<uint8_t> buf(new uint8_t[65536]);
			socketPtr->asyncReceive(&(socketPtr->ipv6socket_), buf);
		}
	}
	return socketPtr;
}

boost::shared_ptr<UdpSocket> UdpSocket::error(const std::string &message) {
	boost::shared_ptr<UdpSocket> socketPtr(new UdpSocket());
	socketPtr->hasError_ = true;
	socketPtr->errorMessage_ = message;
	return socketPtr;
}

std::string UdpSocket::getErrorMessage() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return errorMessage_;
}

bool UdpSocket::hasError() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return hasError_;
}

void UdpSocket::write(const uint8_t *in, size_t size) {
	sendBuffer_->write(in, size);
}

size_t UdpSocket::read(uint8_t *out, size_t size) {
	return receiveBuffer_->read(out, size);
}

std::string UdpSocket::readString(size_t size) {
	return receiveBuffer_->readString(size);
}

size_t UdpSocket::bytesRemaining() const {
	return receiveBuffer_->bytesRemaining();
}

void UdpSocket::setReadpos(size_t pos) {
	return receiveBuffer_->setReadpos(pos);
}

size_t UdpSocket::getSendbufferSize() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	return sendqueue_.getMemSize() + sendBuffer_->size();
}

size_t UdpSocket::getReceivebufferSize() {
	return receiveBuffer_->size();
}

void UdpSocket::setSendbufferLimit(size_t maxSize) {
	sendqueue_.setMemSizeLimit(maxSize);
}

Buffer &UdpSocket::getReceiveBuffer() {
	return *receiveBuffer_;
}

uint16_t UdpSocket::getLocalPort() {
	return localPort_;
}

bool UdpSocket::send(const std::string &host, uint16_t port) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);

	QueueItem item(sendBuffer_, host, port);
	bool datagramsDiscarded = sendqueue_.push(item);
	if(!asyncSendInProgress_) {
		asyncSend();
	}

	sendBuffer_.reset(new Buffer());
	return datagramsDiscarded;
}

void UdpSocket::handleResolve(const boost::system::error_code &error,
		udp::resolver::iterator endpointIterator,
		boost::shared_ptr<Buffer> buffer) {
	handleSend(error ? error : boost::asio::error::host_not_found, buffer, V4FirstIterator<udp>(endpointIterator));
}

bool UdpSocket::receive() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	if (receivequeue_.isEmpty()) {
		receiveBuffer_->clear();
		remoteIp_ = "";
		remotePort_ = 0;
		return false;
	}

	receiveBuffer_ = receivequeue_.peek().buffer;
	remoteIp_ = receivequeue_.peek().remoteHost;
	remotePort_ = receivequeue_.peek().remotePort;
	receivequeue_.pop();
	return true;
}

void UdpSocket::close() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	sendqueue_.clear();
	receivequeue_.clear();
	sendBuffer_->clear();
	receiveBuffer_->clear();
}

std::string UdpSocket::getRemoteIp() {
	return remoteIp_;
}

uint16_t UdpSocket::getRemotePort() {
	return remotePort_;
}

void UdpSocket::asyncSend() {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);

	if (sendqueue_.isEmpty()) {
		return;
	}

	QueueItem item = sendqueue_.peek();
	sendqueue_.pop();
	asyncSendInProgress_ = true;

	boost::system::error_code ec;
	udp::endpoint endpoint(address::from_string(item.remoteHost, ec), item.remotePort);
	if(!ec) {
		handleResolve(ec, udp::resolver::iterator::create(endpoint, "", ""), item.buffer);
	} else {
		udp::resolver::query query(item.remoteHost, boost::lexical_cast<std::string>(item.remotePort), udp::resolver::query::numeric_service | udp::resolver::query::address_configured);
		resolver_.async_resolve(query, boost::bind(&UdpSocket::handleResolve, shared_from_this(),
				boost::asio::placeholders::error, boost::asio::placeholders::iterator, item.buffer));
	}
}

udp::socket *UdpSocket::getAppropriateSocket(const udp::endpoint &endpoint) {
	if (endpoint.protocol() == udp::v4()) {
		return &ipv4socket_;
	} else {
		return &ipv6socket_;
	}
}

void UdpSocket::handleSend(const boost::system::error_code &err, boost::shared_ptr<Buffer> buffer, V4FirstIterator<udp> endpoints) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);

	if(err && endpoints.hasNext()) {
		udp::endpoint endpoint;
		udp::socket *sock;
		do {
			endpoint = endpoints.next();
			sock = getAppropriateSocket(endpoint);
		} while(!sock->is_open() && endpoints.hasNext());

		if(sock->is_open()) {
			sock->async_send_to(
					boost::asio::const_buffers_1(buffer->getData(), buffer->size()),
					endpoint,
					boost::bind(&UdpSocket::handleSend, shared_from_this(), boost::asio::placeholders::error, buffer, endpoints));
			return;
		}
	}

	asyncSendInProgress_ = false;

	if (!sendqueue_.isEmpty()) {
		asyncSend();
	}
}

void UdpSocket::asyncReceive(boost::asio::ip::udp::socket *sock,
		boost::shared_array<uint8_t> recvbuffer) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	boost::shared_ptr<udp::endpoint> endpoint =
			boost::make_shared<udp::endpoint>();
	sock->async_receive_from(
			boost::asio::mutable_buffers_1(recvbuffer.get(), 65536),
			*endpoint,
			boost::bind(&UdpSocket::handleReceive, boost::weak_ptr<UdpSocket>(shared_from_this()),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred, endpoint,
					sock, recvbuffer));
}

void UdpSocket::handleReceive(boost::weak_ptr<UdpSocket> ptr, const boost::system::error_code &err,
		size_t bytesTransferred,
		boost::shared_ptr<boost::asio::ip::udp::endpoint> endpoint,
		boost::asio::ip::udp::socket *sock,
		boost::shared_array<uint8_t> recvbuffer) {
	boost::shared_ptr<UdpSocket> sockPtr = ptr.lock();
	if(!sockPtr) {
		return;
	}

	boost::lock_guard<boost::recursive_mutex> guard(sockPtr->commonMutex_);
	if (!err) {
		boost::shared_ptr<Buffer> buffer = boost::make_shared<Buffer>();
		buffer->write(recvbuffer.get(), bytesTransferred);
		boost::system::error_code ec;
		sockPtr->receivequeue_.push(QueueItem(buffer, endpoint->address().to_string(ec), endpoint->port()));
		sockPtr->asyncReceive(sock, recvbuffer);
	} else if (!sock->is_open()) {
		sockPtr->hasError_ = true;
		sockPtr->errorMessage_ = err.message();
	}
}
