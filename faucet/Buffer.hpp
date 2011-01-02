#pragma once

#include <faucet/Handled.hpp>
#include <faucet/Writable.hpp>

#include <boost/integer.hpp>
#include <vector>
#include <string>
#include <algorithm>

class Buffer : public Handled, public Writable {
	std::vector<uint8_t> data;
	std::vector<uint8_t>::iterator readIterator;
public:
	Buffer() : data(), readIterator(data.begin()) {}

	/**
	 * Empty the buffer.
	 */
	void clear() {
		data.clear();
		readIterator = data.begin();
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
		return data.end()-readIterator;
	}

	/**
	 * Set the read position to the given byte index in the buffer.
	 */
	void setReadpos(size_t pos) {
		if(pos<=data.size()) {
			readIterator = data.begin()+pos;
		} else {
			readIterator = data.end();
		}
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
		std::copy(readIterator, readIterator+size, out);
		readIterator += size;
		return size;
	}

	/**
	 * Append a string to the buffer
	 *
	 * @param str A 0-terminated string
	 * @param withDelimiter Whether or not to copy the
	 * terminating 0 to the buffer
	 */
	void writeString(const char *str, bool withDelimiter) {
		size_t len = strlen(str);
		if(withDelimiter) {
			len++;
		}
		write(reinterpret_cast<const uint8_t *>(str), len);
	}

	/**
	 * Read len characters from the buffer and
	 * return them as a 0-terminated string. If less than len characters
	 * are available, the returned string will be shorter.
	 *
	 * Deleting the pointer is the caller's responsibility.
	 */
	char *readString(size_t len) {
		len = std::min(len, bytesRemaining());
		char *result = new char[len+1];
		read(reinterpret_cast<uint8_t *>(result), len);
		result[len] = 0;
		return result;
	}

	/**
	 * Read a 0-terminated string from the buffer and
	 * return it. If the buffer contains no 0
	 * value, the entire buffer plus an added 0
	 * will be returned in the string.
	 *
	 * Deleting the pointer is the caller's responsibility.
	 */
	char *readString() {
		size_t len = std::find(readIterator, data.end(), 0) - readIterator;
		char *result = readString(len);

		// Remove the separator too, unless we just read the entire buffer
		if(readIterator != data.end()) {
			++readIterator;
		}
		return result;
	}

	/**
	 * Get a pointer to the buffer contents
	 */
	const uint8_t *getData() const {
		return data.data();
	}
};
