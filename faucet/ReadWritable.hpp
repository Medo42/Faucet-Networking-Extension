#pragma once

#include <faucet/clipped_cast.hpp>

#include <boost/integer.hpp>
#include <algorithm>
#include <cmath>
#include <string>

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
	ReadWritable() : littleEndian_(littleEndianDefault_) {}
	virtual ~ReadWritable() {}

	virtual void write(const uint8_t *in, size_t size) = 0;

	virtual size_t read(uint8_t *out, size_t size) = 0;

	/**
	 * Read len characters and return them as a string.
	 *
	 * If less than len characters are available, the returned string
	 * will be shorter than requested.
	 */
	virtual std::string readString(size_t len) = 0;

	virtual size_t bytesRemaining() const = 0;

	virtual void setReadpos(size_t pos) = 0;

	/**
	 * The client is about to write this amount of extra data.
	 * Can be implemented to optimize for this case.
	 */
	virtual void prepareWrite(size_t extraData) { }

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

	void setLittleEndian(bool littleEndian) { littleEndian_ = littleEndian; }
	static void setLittleEndianDefault(bool littleEndianDefault) { littleEndianDefault_ = littleEndianDefault; }
};
