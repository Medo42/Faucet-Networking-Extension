#pragma once

#include <faucet/Asio.hpp>
#include <faucet/V4FirstIterator.hpp>
#include "ConnectionState.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

class TcpConnecting: public ConnectionState {
public:
	TcpConnecting(TcpSocket &socket);
	void enter(const char *host, uint16_t port);
	virtual void abort();
	virtual bool isConnecting() {
		return true;
	}
	virtual bool allowWrite() {
		return true;
	}
	virtual bool isEof() {
		return false;
	}
private:
	boost::asio::ip::tcp::resolver resolver;
	bool abortRequested;

	typedef boost::asio::ip::tcp::resolver::protocol_type protocol_type;
	void handleResolve(boost::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			boost::asio::ip::tcp::resolver::iterator endpointIterator);

	boost::system::error_code startConnectionAttempt(boost::shared_ptr<TcpSocket> tcpSocket,
			V4FirstIterator<boost::asio::ip::tcp> endpoints, boost::system::error_code &ec);

	void handleConnect(boost::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			V4FirstIterator<boost::asio::ip::tcp> endpoints);
};
