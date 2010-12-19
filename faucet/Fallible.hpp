#pragma once

#include <faucet/Handled.hpp>
#include <string>

class Fallible : public Handled {
public:
	virtual bool hasError() = 0;
	virtual const std::string &getErrorMessage() = 0;
};
