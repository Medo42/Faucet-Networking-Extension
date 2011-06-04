#pragma once

#include <faucet/Asio.hpp>

#include <shb/AbstractBuffer.hpp>
#include <boost/integer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost::asio::ip;

class UdpMessage;

/**
 * This class is a placeholder implementation to allow UDP sending.
 * Once a proper UDP api has been defined it will probably become obsolete.
 */
class UdpSender : public boost::enable_shared_from_this<UdpSender> {
public:
	UdpSender();
	void send(shb::AbstractBuffer& buffer, std::string host, uint16_t port);

private:
	udp::socket ipv4Socket;
	udp::socket ipv6Socket;
	udp::resolver resolver;

	void handleResolve(const boost::system::error_code &error,
			udp::resolver::iterator endpointIterator,
			boost::shared_ptr<UdpMessage> message);

	void handleSend(const boost::system::error_code &error,
			udp::resolver::iterator endpointIterator,
			boost::shared_ptr<UdpMessage> message);

	udp::socket *getAppropriateSocket(udp::endpoint &endpoint);
};
