#pragma once

#include "ConnectionState.hpp"

class TcpClosing : public ConnectionState {
	virtual void abort(TcpSocket &socket);
	virtual bool allowWrite() { return false; }
	virtual bool isEof(TcpSocket &socket);
};
