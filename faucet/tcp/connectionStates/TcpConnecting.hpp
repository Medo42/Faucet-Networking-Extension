#pragma once

#include <faucet/Asio.hpp>
#include "ConnectionState.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

class TcpConnecting: public ConnectionState {
public:
	TcpConnecting();
	void enter(TcpSocket &socket, const char *host, uint16_t port);
	virtual void abort(TcpSocket &socket);
	virtual bool isConnecting() { return true; }
	virtual bool allowWrite() { return true; }
	virtual bool isEof(TcpSocket &socket);
private:
	boost::asio::ip::tcp::resolver resolver;
	bool abortRequested;

	void handleResolve(boost::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			boost::asio::ip::tcp::resolver::iterator endpointIterator);

	void startConnectionAttempt(boost::shared_ptr<TcpSocket> tcpSocket,
			boost::asio::ip::tcp::resolver::iterator endpointIterator);

	void handleConnect(boost::shared_ptr<TcpSocket> tcpSocket,
			const boost::system::error_code &err,
			boost::asio::ip::tcp::resolver::iterator endpointIterator);
};
