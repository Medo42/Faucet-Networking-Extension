#pragma once

#include <faucet/Asio.hpp>
#include <faucet/Fallible.hpp>

#include <boost/integer.hpp>
#include <boost/thread.hpp>
#include <string>
#include <memory>

class TcpSocket;

using namespace boost::asio::ip;

// TODO: Seperate error reporting for ipv4 and ipv6
class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor> {
public:
	static std::shared_ptr<TcpAcceptor> listen(std::shared_ptr<tcp::acceptor> acceptor);
	static std::shared_ptr<TcpAcceptor> error(std::string message);

	virtual ~TcpAcceptor();

	virtual std::string getErrorMessage();
	virtual bool hasError();

	/**
	 * If a connection is waiting to be accepted, a socket to this
	 * connection is returned. Otherwise a NULL pointer is returned.
	 */
	std::shared_ptr<TcpSocket> accept();

	/**
	 * Stop listening for connections. Call this to ensure the
	 * object is actually destroyed when the last external shared_ptr
	 * reference is released.
	 */
	void close();

private:
	TcpAcceptor();

	std::shared_ptr<tcp::socket> socket_;
	std::shared_ptr<tcp::acceptor> acceptor_;

	bool hasError_;
	std::string errorMessage_;

	boost::recursive_mutex socketMutex_;
	boost::recursive_mutex errorMutex_;

	void startAsyncAccept();
	void handleAccept(const boost::system::error_code &error, std::shared_ptr<tcp::socket> socket);
};
