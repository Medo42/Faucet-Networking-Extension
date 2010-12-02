#pragma once

#include <boost/integer.hpp>
#include <set>

/**
 * Generates unique integer values
 */
class HandlePool {
public:
	HandlePool();
	~HandlePool() {};

	uint32_t allocate();
	void release(uint32_t handle);

private:
	std::set<uint32_t> usedHandles;
	uint32_t nextHandle;
};

HandlePool::HandlePool() :
	usedHandles(),
	nextHandle(0) {
}

uint32_t HandlePool::allocate() {
	while(usedHandles.count(nextHandle) > 0) {
		nextHandle++;
	}
	usedHandles.insert(nextHandle);
	return nextHandle++;
}

void HandlePool::release(uint32_t handle) {
	usedHandles.erase(handle);
}
