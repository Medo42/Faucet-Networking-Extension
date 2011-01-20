#pragma once

#include <faucet/clipped_cast.hpp>

#include <boost/integer.hpp>
#include <algorithm>
#include <cmath>

class ReadWritable {
private:
	static bool littleEndianDefault_;
	bool littleEndian_;

	void writeByteOrderAware(uint8_t *buffer, size_t size) {
		if(!littleEndian_) {
			std::reverse(buffer, buffer+size);
		}
		write(buffer, size);
	}

public:
	virtual void write(const uint8_t *in, size_t size) = 0;
	virtual size_t read(uint8_t *out, size_t size) = 0;
	virtual size_t bytesRemaining() const = 0;

	/**
	 * Convert a double to the target integer type and write it to the
	 * given writable. Fractional numbers are rounded to the nearest integer.
	 * Values outside of the target type's range will be clamped to the border
	 * values of the type.
	 */
	template<typename IntType>
	void writeIntValue(double value) {
		IntType converted = clipped_cast<IntType>(round(value));
		writeByteOrderAware(reinterpret_cast<uint8_t *>(&converted), sizeof(converted));
	}

	void writeFloat(double value) {
		float converted = clipped_cast<float>(value);
		writeByteOrderAware(reinterpret_cast<uint8_t *>(&converted), sizeof(converted));
	}

	void writeDouble(double value) {
		writeByteOrderAware(reinterpret_cast<uint8_t *>(&value), sizeof(value));
	}

	template <typename DesiredType>
	double readValue() {
		DesiredType value;
		uint8_t *valueAsBuffer = reinterpret_cast<uint8_t *>(&value);
		read(valueAsBuffer, sizeof(DesiredType));
		if(!littleEndian_) {
			std::reverse(valueAsBuffer, valueAsBuffer + sizeof(DesiredType));
		}
		return static_cast<double>(value);
	}

	/**
	 * Read len characters and return them as a 0-terminated string.
	 *
	 * If less than len characters are available, the returned string
	 * will be shorter.
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

	ReadWritable() : littleEndian_(littleEndianDefault_) {}
	void setLittleEndian(bool littleEndian) { littleEndian_ = littleEndian; }
	static void setLittleEndianDefault(bool littleEndianDefault) { littleEndianDefault_ = littleEndianDefault; }
};
