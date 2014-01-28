#pragma once

#include <faucet/Handled.hpp>
#include <faucet/ReadWritable.hpp>

#include <boost/integer.hpp>
#include <boost/utility.hpp>

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

class Buffer : public Handled, public ReadWritable, boost::noncopyable {
	std::vector<uint8_t> data;
	size_t readIndex;
public:
	Buffer() : data(), readIndex(0) {}

	/**
	 * Empty the buffer.
	 */
	void clear() {
		data.clear();
		readIndex = 0;
	}

	/**
	 * Return the number of bytes currently in the buffer.
	 */
	size_t size() const {
		return data.size();
	}

	/**
	 * Return the number of bytes still remaining to be read.
	 */
	size_t bytesRemaining() const {
		return size()-readIndex;
	}

	/**
	 * Set the read position to the given byte index in the buffer.
	 */
	void setReadpos(size_t pos) {
		if(pos<=data.size()) {
			readIndex = pos;
		} else {
			readIndex = size();
		}
	}

    size_t getReadpos() const {
        return readIndex;
    }

	/**
	 * Append the given array to the end of the buffer.
	 */
	void write(const uint8_t *in, size_t size) {
		data.insert(data.end(), in, in+size);
	}

	/**
	 * Copy size bytes from the current read position of the buffer
	 * into the provided array. If less data is available,
	 * as much data as available is copied, and the rest of the
	 * provided array is not changed. The number of bytes
	 * actually read is returned.
	 */
	size_t read(uint8_t* out, size_t size) {
		size = std::min(size, bytesRemaining());
		memcpy(out, data.data()+readIndex, size);
		readIndex += size;
		return size;
	}

	/**
	 * Copy size bytes from the current read position of the buffer
	 * into a newly allocated string. If less data is available,
	 * the string will contain the entire remainder of the buffer.
	 */
	std::string readString(size_t size) {
		size = std::min(size, bytesRemaining());
		char *stringStart = (char*)data.data()+readIndex;
		readIndex += size;
		return std::string(stringStart, size);
	}

	void prepareWrite(size_t extraData) {
		data.reserve(data.size() + extraData);
	}

	/**
	 * Get a pointer to the buffer contents
	 */
	const uint8_t *getData() const {
		return data.data();
	}
};
