#pragma once
#include "Socket.h"
#include "inttypes.h"

class TcpState;

class TcpSocket : public Socket {
public:
	virtual ~TcpSocket();

	/**
	 * Create a new socket representing a connection to the
	 * given host and port.
	 */
	static TcpSocket *connectTo(char *numericAddress, uint16_t port);
private:
	TcpSocket();
	TcpState *state;
};
