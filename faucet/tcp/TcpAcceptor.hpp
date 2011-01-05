#pragma once

#include <faucet/Asio.hpp>
#include <faucet/Fallible.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

class TcpSocket;

using namespace boost::asio::ip;
class TcpAcceptor : public boost::enable_shared_from_this<TcpAcceptor> {
public:
	virtual ~TcpAcceptor();

	virtual const std::string &getErrorMessage();
	virtual bool hasError();

	/**
	 * If a connection is waiting to be accepted, a socket to this
	 * connection is returned. Otherwise a NULL pointer is returned.
	 */
	boost::shared_ptr<TcpSocket> accept();

	/**
	 * Create a new Acceptor that will listen at the given port.
	 */
	static boost::shared_ptr<TcpAcceptor> listen(const tcp::endpoint &endpoint);
private:
	TcpAcceptor();
	tcp::acceptor acceptor_;

	/**
	 * Between method calls socket_ always points to a socket that has
	 * *not* been returned yet. Thus is belongs to this object and has
	 * to be destroyed properly on destruction.
	 */
	tcp::socket *socket_;
	bool socketIsConnected_;

	bool hasError_;
	std::string errorMessage_;

	void handleAccept(const boost::system::error_code &error);
};
