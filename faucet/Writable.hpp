#pragma once

#include <boost/integer.hpp>

class Writable {
public:
	virtual void write(const uint8_t *buffer, size_t size) = 0;
};
