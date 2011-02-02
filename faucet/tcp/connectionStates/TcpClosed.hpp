#pragma once

#include "ConnectionState.hpp"

#include <string>

class TcpClosed: public ConnectionState {
private:
	bool hasError;
	std::string errorMessage;

public:
	TcpClosed(TcpSocket &tcpSocket);
	void enterError(const std::string &error);
	void enterClosed();

	virtual void abort();

	virtual bool isErrorState() {
		return hasError;
	}
	virtual std::string getErrorMessage() {
		return errorMessage;
	}
	virtual bool allowWrite() {
		return false;
	}
	virtual bool isEof() {
		return true;
	}
};
