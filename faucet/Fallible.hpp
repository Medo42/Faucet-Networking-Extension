#pragma once

#include <faucet/Handled.hpp>
#include <string>

class Fallible : public Handled {
public:
	virtual ~Fallible() {}

	virtual bool hasError() = 0;
	virtual std::string getErrorMessage() = 0;
};
