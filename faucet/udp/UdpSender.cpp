#include "UdpSender.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <cstring>

class UdpMessage {
public:
	UdpMessage(size_t sourceSize, const uint8_t *source) : size(sourceSize), buffer(new uint8_t[size]) {
		memcpy(buffer, source, size);
	}

	~UdpMessage() {
		delete[] buffer;
	}

	boost::asio::mutable_buffers_1 getBuffer() {
		return boost::asio::buffer(buffer, size);
	}
private:
	size_t size;
	uint8_t *buffer;
};

UdpSender::UdpSender() :
		ipv4Socket(Asio::getIoService()),
		ipv6Socket(Asio::getIoService()),
		resolver(Asio::getIoService()) {
	boost::system::error_code error;
	ipv4Socket.open(udp::v4(), error);
	ipv6Socket.open(udp::v6(), error);
}

void UdpSender::send(boost::shared_ptr<Buffer> buffer, std::string host, uint16_t port) {
	udp::resolver::query query(host, boost::lexical_cast<std::string>(port), tcp::resolver::query::numeric_service);
	boost::shared_ptr<UdpMessage> message(new UdpMessage(buffer->size(), buffer->getData()));
	resolver.async_resolve(query, boost::bind(
			&UdpSender::handleResolve,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator,
			message));
}

void UdpSender::handleResolve(const boost::system::error_code &error,
		udp::resolver::iterator endpointIterator,
		boost::shared_ptr<UdpMessage> message) {
	if(!error) {
		boost::system::error_code hostNotFound = boost::asio::error::host_not_found;
		handleSend(hostNotFound, endpointIterator, message);
	}
}

// TODO ipv4 first
void UdpSender::handleSend(const boost::system::error_code &error,
		udp::resolver::iterator endpointIterator,
		boost::shared_ptr<UdpMessage> message) {
	if(error && endpointIterator != udp::resolver::iterator()) {
		udp::endpoint endpoint = *endpointIterator;
		udp::socket *socket = getAppropriateSocket(endpoint);
		if(socket->is_open()) {
			socket->async_send_to(message->getBuffer(), endpoint, boost::bind(
					&UdpSender::handleSend,
					shared_from_this(),
					boost::asio::placeholders::error,
					++endpointIterator,
					message));
		} else {
			handleSend(error, ++endpointIterator, message);
		}
	}
}

udp::socket *UdpSender::getAppropriateSocket(udp::endpoint &endpoint) {
	if(endpoint.protocol() == udp::v4()) {
		return &ipv4Socket;
	} else {
		return &ipv6Socket;
	}
}
