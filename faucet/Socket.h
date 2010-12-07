#pragma once

#include <faucet/Fallible.hpp>

#include <string>

class Socket : public Fallible {
public:
	virtual ~Socket(){};

	virtual bool isConnecting() = 0;
};
