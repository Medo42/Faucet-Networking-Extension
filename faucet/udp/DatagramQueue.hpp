#pragma once

#include <faucet/Buffer.hpp>

#include <memory>
#include <string>
#include <deque>

struct QueueItem {
	QueueItem(std::shared_ptr<Buffer> buffer,
			std::string hostname, uint16_t port) :
			memSize(sizeof(QueueItem)+buffer->size()), buffer(buffer), remoteHost(hostname), remotePort(port) {
	}

	const size_t memSize;
	std::shared_ptr<Buffer> buffer;
	std::string remoteHost;
	uint16_t remotePort;
};

// TODO check how much copying goes on here, how move semantics might help
class DatagramQueue {
private:
	static const size_t DEFAULT_MEM_LIMIT = 2*1024*1024;
	size_t memSize_;
	size_t memSizeLimit_;
	std::deque<QueueItem> queue_;

public:
	DatagramQueue() : memSize_(0), memSizeLimit_(DEFAULT_MEM_LIMIT), queue_() {}

	bool push(const QueueItem& item) {
		bool datagramsDiscarded = false;
		/*
		 * The idea here is quite simple: We throw away the new datagram if it can't
		 * possibly fit into the queue, otherwise we discard the oldest datagrams
		 * from the queue to make space for the new one.
		 */
		if(memSizeLimit_ < item.memSize) {
			return true;
		}

		while(memSize_ > memSizeLimit_ || memSizeLimit_ - memSize_ < item.memSize) {
			pop();
			datagramsDiscarded = true;
		}

		queue_.push_back(item);
		memSize_ += item.memSize;
		return datagramsDiscarded;
	}

	QueueItem& peek() {
		return queue_.front();
	}

	void pop() {
		if(!queue_.empty()) {
			memSize_ -= queue_.front().memSize;
			queue_.pop_front();
		}
	}

	bool isEmpty() {
		return queue_.empty();
	}

	void clear() {
		queue_.clear();
		memSize_ = 0;
	}

	size_t getMemSize() {
		return memSize_;
	}

	void setMemSizeLimit(size_t limit) {
		memSizeLimit_ = limit;
		while(memSize_ > memSizeLimit_) {
			pop();
		}
	}
};
