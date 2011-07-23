#include "GmStringBuffer.hpp"

std::string stringReturnBuffer = std::string();

const char *replaceStringReturnBuffer(const std::string &str) {
	stringReturnBuffer.assign(str);
	return stringReturnBuffer.c_str();
}
