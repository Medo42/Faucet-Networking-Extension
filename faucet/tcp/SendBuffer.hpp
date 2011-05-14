#pragma once

#include <faucet/Asio.hpp>
#include <boost/integer.hpp>
#include <vector>
#include <cstdlib>

class SendBuffer {
private:
	static const size_t BUFFER_SIZE = 65536;
	std::vector<uint8_t *> buffers;
	size_t firstElementIndex;
	size_t freeSpaceInLastBuffer;
	uint8_t *endPtr;
	size_t committedBytes;

public:
	SendBuffer() :
		buffers(),
		firstElementIndex(0),
		freeSpaceInLastBuffer(0),
		endPtr(0),
		committedBytes(0) {}

	~SendBuffer() {
		for(size_t i=0; i<buffers.size(); i++) {
			delete[] buffers[i];
		}
	}

	/**
	 * Return all committed bytes as a ConstBufferSequence
	 */
	std::vector<boost::asio::const_buffer> getCommittedData() const {
		std::vector<boost::asio::const_buffer> result;
		result.reserve(buffers.size());
		size_t bytesRemaining = committedBytes;

		for(size_t i=0; i<buffers.size() && bytesRemaining>0; i++) {
			uint8_t *buffer = buffers[i];
			size_t size = BUFFER_SIZE;
			if(i==0) {
				buffer += firstElementIndex;
				size -= firstElementIndex;
			}
			if(i == buffers.size()-1) {
				size -= freeSpaceInLastBuffer;
			}
			if(size >= bytesRemaining) {
				size = bytesRemaining;
			}
			result.push_back(boost::asio::const_buffer(buffer, size));
			bytesRemaining -= size;
		}
		return result;
	}

	size_t totalSize() const {
		return buffers.size()*BUFFER_SIZE - freeSpaceInLastBuffer - firstElementIndex;
	}

	size_t committedSize() const {
		return committedBytes;
	}

	/**
	 * Commit everything currently in the buffer
	 */
	void commit() {
		committedBytes = totalSize();
	}

	/**
	 * Add uncommitted data to the end of the buffer
	 */
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

	/**
	 * Remove size committed bytes from the beginning of the buffer.
	 */
	void pop(size_t size) {
		if(size>committedBytes) {
			throw std::out_of_range("Attempted to pop uncommitted data from a SendBuffer.");
		}
		committedBytes -= size;
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

	void clear() {
		for(size_t i=0; i<buffers.size(); i++) {
			delete[] buffers[i];
		}
		buffers.clear();
		firstElementIndex = 0;
		freeSpaceInLastBuffer = 0;
		committedBytes = 0;
	}
};
