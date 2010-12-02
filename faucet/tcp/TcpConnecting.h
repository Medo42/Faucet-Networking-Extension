#pragma once
#include <faucet/tcp/TcpState.h>
#include <faucet/asio.h>
#include <boost/integer.hpp>
#include <string>

class TcpConnecting : public TcpState{
public:
	TcpConnecting(const std::string &host, uint16_t port);
	virtual ~TcpConnecting();

private:
	boost::asio::ip::tcp::resolver resolver;
};
