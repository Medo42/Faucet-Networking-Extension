#pragma once
#include <faucet/Socket.h>
#include <faucet/asio.h>

#include <boost/integer.hpp>
#include <string>

using namespace boost::asio::ip;

class TcpSocket : public Socket {
public:
	virtual ~TcpSocket();

	virtual bool isConnecting();
	virtual const std::string &getErrorMessage();
	virtual bool hasError();

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static TcpSocket *connectTo(const char *numericAddress, uint16_t port);

	/**
	 * Create an error socket with the given parameter as error message
	 */
	static TcpSocket *error(const std::string &message);
private:
	enum State {
		TCPSOCK_CONNECTING,
		TCPSOCK_CONNECTED,
		TCPSOCK_CLOSED,
		TCPSOCK_FAILED
	};

	tcp::socket socket_;
	tcp::resolver resolver_;

	State state_;
	std::string errorMessage_;

	TcpSocket(State initialState);
	void handleError(const boost::system::error_code &err);
	void handleResolve(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);

	void handleConnect(const boost::system::error_code &err,
			tcp::resolver::iterator endpointIterator);

};
