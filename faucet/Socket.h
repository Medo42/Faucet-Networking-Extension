#pragma once

#include <string>

class Socket {
public:
	virtual ~Socket(){};

	virtual bool isConnecting() = 0;
	virtual bool hasError() = 0;
	virtual const std::string &getErrorMessage() = 0;
};
