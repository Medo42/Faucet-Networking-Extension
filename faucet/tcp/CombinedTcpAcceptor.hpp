#pragma once

#include <faucet/Fallible.hpp>
#include <faucet/tcp/TcpAcceptor.hpp>

#include <boost/integer.hpp>

/**
 * This class combines two TcpAcceptors, one for IPv4 and once for IPv6.
 * Both will listen on the same port, but on top their respective protocol.
 * The intention is to hide IPv4/IPv6 differences from the library user
 * and allow dual stack operation.
 */
class CombinedTcpAcceptor : public Fallible {
public:
	CombinedTcpAcceptor(uint16_t port);
	virtual ~CombinedTcpAcceptor();

	virtual const std::string &getErrorMessage();
	virtual bool hasError();

	/**
	 * If a connection is waiting to be accepted, a socket to this
	 * connection is returned. Otherwise a NULL pointer is returned.
	 */
	TcpSocket *accept();
private:
	TcpAcceptor v4Acceptor_, v6Acceptor_;
	bool checkV6First_;
};
