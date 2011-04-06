#include "GmStringBuffer.hpp"

std::string stringReturnBuffer;

const char *replaceStringReturnBuffer(const std::string &str) {
	stringReturnBuffer.assign(str);
	return stringReturnBuffer.c_str();
}
