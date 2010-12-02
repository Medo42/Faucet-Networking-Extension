#pragma once

#include "TcpState.h"
#include <string>

class TcpClosed: public TcpState {
public:
	TcpClosed(const std::string &errorMessage);
	TcpClosed();
	virtual ~TcpClosed();

private:
	bool closedByError;
	std::string errorMessage;
};
