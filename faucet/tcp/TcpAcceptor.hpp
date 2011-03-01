#pragma once

#include <faucet/Asio.hpp>
#include <faucet/Fallible.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <string>

class TcpSocket;

using namespace boost::asio::ip;
using boost::shared_ptr;
class TcpAcceptor : public boost::enable_shared_from_this<TcpAcceptor> {
public:
	virtual ~TcpAcceptor();

	virtual std::string getErrorMessage();
	virtual bool hasError();

	/**
	 * If a connection is waiting to be accepted, a socket to this
	 * connection is returned. Otherwise a NULL pointer is returned.
	 */
	shared_ptr<TcpSocket> accept();

	/**
	 * Stop listening for connections. Call this to ensure the
	 * object is actually destroyed when the last external shared_ptr
	 * reference is released.
	 */
	void close();

	/**
	 * Create a new Acceptor that will listen at the given port.
	 */
	static shared_ptr<TcpAcceptor> listen(const tcp::endpoint &endpoint);
private:
	TcpAcceptor();
	tcp::acceptor acceptor_;

	shared_ptr<tcp::socket> socket_;

	bool hasError_;
	std::string errorMessage_;

	boost::mutex socketMutex_;
	boost::mutex errorMutex_;

	void startAsyncAccept();
	void handleAccept(const boost::system::error_code &error, shared_ptr<tcp::socket> socket);
};
