#pragma once

#include <string>

class Fallible {
public:
	virtual bool hasError() = 0;
	virtual const std::string &getErrorMessage() = 0;
};
