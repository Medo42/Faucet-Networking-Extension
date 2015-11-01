#include "UdpSocket.hpp"

#include "broadcastAddrs.hpp"

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
 * and IPv6 in WinXP. Further, Vista's dual stack sockets don't work as expected with
 * regard to port re-use. Specifically, they apparently allow you to bind another pure
 * IPv4 and/or IPv6 socket to the same port within your application.
 *
 * To work around these problems, this is now done the following way:
 *
 * If the port number is not 0, we simply try binding an IPv4 and IPv6 socket to the given
 * port. If it is 0 and we are on Vista/7, we bind a dual stack socket first just to get
 * a port number assigned from the operating system which is free for both IPv4 and IPv6.
 * We remember the port number and then close that socket again, and then proceed just as
 * if that port had been given in the first place.
 * On WinXP with port 0, the dual stack socket creation will fail, and we just attempt to
 * open an IPv4 socket with an unused port, and then an IPv6 socket on the same port.
 *
 * As a consequence, this operation is not reliable on WinXP, since the IPv6 part may fail
 * even though there might be free ports available. It's also not reliable on Vista and
 * later because another process might steal the port after we close the dual stack socket,
 * but before we bind the other sockets. However, that seems rather unlikely in real-world
 * scenarios.
 */
std::shared_ptr<UdpSocket> UdpSocket::bind(uint16_t portnr) {
	std::shared_ptr<UdpSocket> socketPtr(new UdpSocket());
	boost::system::error_code ignoredError, v4Error, v6Error;

	if (portnr == 0) {
		try {
		    udp::socket dualstack_placeholder(Asio::getIoService());
			dualstack_placeholder.open(udp::v6());
			dualstack_placeholder.set_option(v6_only(false));
			dualstack_placeholder.bind(udp::endpoint(udp::v6(), portnr));
			portnr = dualstack_placeholder.local_endpoint().port();
			dualstack_placeholder.close(ignoredError);
		} catch (boost::system::system_error &e) {
			// Error -> Probably no dual stack support or no v6 support at all
		}
	}

	try {
		socketPtr->ipv4socket_.open(udp::v4());
		socketPtr->ipv4socket_.set_option(udp::socket::broadcast(true));
		socketPtr->ipv4socket_.bind(udp::endpoint(udp::v4(), portnr));
		portnr = socketPtr->ipv4socket_.local_endpoint().port();
	} catch (boost::system::system_error &e) {
		// Error -> IPv4 port is probably in use
		v4Error = e.code();
		socketPtr->ipv4socket_.close(ignoredError);
	}

    try {
        socketPtr->ipv6socket_.open(udp::v6());
        socketPtr->ipv6socket_.set_option(udp::socket::broadcast(true));
        socketPtr->ipv6socket_.set_option(v6_only(true), ignoredError);
        socketPtr->ipv6socket_.bind(udp::endpoint(udp::v6(), portnr));
        portnr = socketPtr->ipv6socket_.local_endpoint().port();
    } catch (boost::system::system_error &e) {
        v6Error = e.code();
        socketPtr->ipv6socket_.close(ignoredError);
    }

	if (!socketPtr->ipv4socket_.is_open()
			&& !socketPtr->ipv6socket_.is_open()) {
		socketPtr->hasError_ = true;
		socketPtr->errorMessage_ = "IPv4: " + v4Error.message() + ", IPv6:"
				+ v6Error.message();
	} else {
		socketPtr->localPort_ = portnr;
		if (socketPtr->ipv4socket_.is_open()) {
            auto buf = std::make_shared<std::array<uint8_t, 65536>>();
			socketPtr->asyncReceive(&(socketPtr->ipv4socket_), buf);
		}
		if (socketPtr->ipv6socket_.is_open()) {
		    auto buf = std::make_shared<std::array<uint8_t, 65536>>();
			socketPtr->asyncReceive(&(socketPtr->ipv6socket_), buf);
		}
	}

	return socketPtr;
}

std::shared_ptr<UdpSocket> UdpSocket::error(const std::string &message) {
	std::shared_ptr<UdpSocket> socketPtr(new UdpSocket());
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

bool UdpSocket::broadcast(uint16_t port) {
	std::vector<boost::asio::ip::address_v4> addrs = findLocalBroadcastAddresses();

	bool anyDiscarded = false;
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);

	for(size_t i=0; i<addrs.size(); i++) {
		QueueItem item(sendBuffer_, addrs[i].to_string(), port);
		anyDiscarded |= sendqueue_.push(item);
	}

	if(!asyncSendInProgress_) {
		asyncSend();
	}

	sendBuffer_.reset(new Buffer());

	return anyDiscarded;
}

void UdpSocket::handleResolve(const boost::system::error_code &error,
		udp::resolver::iterator endpointIterator,
		std::shared_ptr<Buffer> buffer) {
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

void UdpSocket::handleSend(const boost::system::error_code &err, std::shared_ptr<Buffer> buffer, V4FirstIterator<udp> endpoints) {
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
		std::shared_ptr<std::array<uint8_t, 65536>> recvbuffer) {
	boost::lock_guard<boost::recursive_mutex> guard(commonMutex_);
	auto endpoint = std::make_shared<udp::endpoint>();
	sock->async_receive_from(
			boost::asio::mutable_buffers_1(recvbuffer->data(), recvbuffer->size()),
			*endpoint,
			boost::bind(&UdpSocket::handleReceive, std::weak_ptr<UdpSocket>(shared_from_this()),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred, endpoint,
					sock, recvbuffer));
}

void UdpSocket::handleReceive(std::weak_ptr<UdpSocket> ptr, const boost::system::error_code &err,
		size_t bytesTransferred,
		std::shared_ptr<boost::asio::ip::udp::endpoint> endpoint,
		boost::asio::ip::udp::socket *sock,
		std::shared_ptr<std::array<uint8_t, 65536>> recvbuffer) {
	std::shared_ptr<UdpSocket> sockPtr = ptr.lock();
	if(!sockPtr) {
		// The UdpSocket has been destroyed or has errored out, no need to keep receiving
		return;
	}

	boost::lock_guard<boost::recursive_mutex> guard(sockPtr->commonMutex_);
	if (!err) {
		auto buffer = std::make_shared<Buffer>();
		buffer->write(recvbuffer->data(), bytesTransferred);
		boost::system::error_code ec;
		sockPtr->receivequeue_.push(QueueItem(buffer, endpoint->address().to_string(ec), endpoint->port()));
	}

	if(sock->is_open()) {
		sockPtr->asyncReceive(sock, recvbuffer);
	}
}
