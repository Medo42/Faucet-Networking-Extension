#pragma once

#include "ConnectionState.hpp"

#include <string>

class TcpError : public ConnectionState {
public:
	TcpError();
	void enter(TcpSocket &socket, const std::string &error);

	virtual void abort(TcpSocket &socket);
	virtual bool isErrorState() { return true; }
	virtual std::string getErrorMessage() { return errorMessage; }
	virtual bool allowWrite() { return false; }
	virtual bool isEof(TcpSocket &socket);
private:
	std::string errorMessage;
};
