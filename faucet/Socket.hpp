#pragma once

#include <faucet/Fallible.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/Buffer.hpp>
#include <string>

class Socket : public Fallible, public ReadWritable {
public:
	virtual ~Socket(){};

	virtual size_t getSendbufferSize() = 0;
	virtual size_t getReceivebufferSize() = 0;
	virtual void setSendbufferLimit(size_t maxSize) = 0;
	virtual Buffer &getReceiveBuffer() = 0;

	virtual std::string getRemoteIp() = 0;
	virtual uint16_t getRemotePort() = 0;
	virtual uint16_t getLocalPort() = 0;
};
