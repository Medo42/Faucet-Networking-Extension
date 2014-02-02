#pragma once

#include <faucet/Asio.hpp>
#include <faucet/V4FirstIterator.hpp>
#include "ConnectionState.hpp"

#include <string>
#include <memory>

class TcpConnecting: public ConnectionState {
public:
	TcpConnecting(TcpSocket &socket);
	virtual ~TcpConnecting() {}

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
	virtual bool setNoDelay(bool noDelay);
private:
	boost::asio::ip::tcp::resolver resolver;
	bool abortRequested;
	bool noDelay_;

	typedef boost::asio::ip::tcp::resolver::protocol_type protocol_type;
	void handleResolve(std::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			boost::asio::ip::tcp::resolver::iterator endpointIterator);

	boost::system::error_code startConnectionAttempt(std::shared_ptr<TcpSocket> tcpSocket,
			V4FirstIterator<boost::asio::ip::tcp> endpoints, boost::system::error_code &ec);

	void handleConnect(std::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			V4FirstIterator<boost::asio::ip::tcp> endpoints);
};
