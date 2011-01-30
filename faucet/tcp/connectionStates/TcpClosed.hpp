#pragma once

#include "ConnectionState.hpp"

class TcpClosed : public ConnectionState {
public:
	virtual void abort(TcpSocket &socket);
	virtual bool allowWrite() { return false; }
	virtual bool isEof(TcpSocket &socket);
};
