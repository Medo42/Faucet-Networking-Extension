#pragma once

#include <boost/integer.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <cstdlib>

class SendBuffer {
private:
	static const size_t BUFFER_SIZE = 65536;
	std::vector<uint8_t *> buffers;
	size_t firstElementIndex;
	size_t freeSpaceInLastBuffer;
	uint8_t *endPtr;

public:
	SendBuffer() : firstElementIndex(0), freeSpaceInLastBuffer(0) {}

	~SendBuffer() {
		for(size_t i=0; i<buffers.size(); i++) {
			delete[] buffers[i];
		}
	}

	std::vector<boost::asio::const_buffer> asConstBufferSequence() const {
		std::vector<boost::asio::const_buffer> result;
		result.reserve(buffers.size());

		for(size_t i=0; i<buffers.size(); i++) {
			uint8_t *buffer = buffers[i];
			size_t size = BUFFER_SIZE;
			if(i==0) {
				buffer += firstElementIndex;
				size -= firstElementIndex;
			}
			if(i == buffers.size()-1) {
				size -= freeSpaceInLastBuffer;
			}
			result.push_back(boost::asio::const_buffer(buffer, size));
		}
		return result;
	}

	size_t size() const {
		return buffers.size()*BUFFER_SIZE - freeSpaceInLastBuffer - firstElementIndex;
	}

	void push(const uint8_t *data, size_t size) {
		while(size > freeSpaceInLastBuffer) {
			memmove(endPtr, data, freeSpaceInLastBuffer);
			data += freeSpaceInLastBuffer;
			size -= freeSpaceInLastBuffer;
			endPtr = new uint8_t[BUFFER_SIZE];
			buffers.push_back(endPtr);
			freeSpaceInLastBuffer = BUFFER_SIZE;
		}

		memmove(endPtr, data, size);
		endPtr += size;
		freeSpaceInLastBuffer -= size;
	}

	template<typename PodType>
	void push(const PodType &podObj) {
		push(reinterpret_cast<const uint8_t*>(&podObj), sizeof(PodType));
	}

	void pop(size_t size) {
		if(size>this->size()) {
			throw std::out_of_range("Attempted to pop more data from a SendBuffer than it contains.");
		}
		div_t emptyBuffersAndBytes = div(firstElementIndex+size, BUFFER_SIZE);
		int emptyBuffers = emptyBuffersAndBytes.quot;
		firstElementIndex = emptyBuffersAndBytes.rem;

		if(emptyBuffers > 0) {
			for(int i=0; i<emptyBuffers; i++) {
				delete[] buffers[i];
			}
			buffers.erase(buffers.begin(), buffers.begin()+emptyBuffers);
		}
	}
};
