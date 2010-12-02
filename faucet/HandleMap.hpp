#pragma once

#include <map>
#include <boost/integer.hpp>
#include "HandlePool.hpp"

/**
 * Provides unique handles to objects and allows access by that handle.
 *
 * All HandleManagers that use the same HandlePool will hold disjoint
 * sets of handles.
 */
template<typename Handled>
class HandleMap {
public:
	HandleMap(HandlePool *pool);

	/**
	 * Associate the element with a pool-unique handle, which is returned.
	 */
	uint32_t allocate(Handled *element);

	/**
	 * Return the element associated with the given handle, or a null pointer
	 * if this Manager does not hold the handle.
	 *
	 * Note that you will also get NULL as result if the given handle has
	 * previously been associated with NULL.
	 */
	Handled *find(uint32_t handle);

	/**
	 * Release the handle-element association.
	 */
	void release(uint32_t handle);

private:
	HandlePool *handlePool_;
	std::map<uint32_t, Handled *> content_;
};


template<typename Handled>
HandleMap<Handled>::HandleMap(HandlePool *pool) :
		handlePool_(pool), content_() {}

template<typename Handled>
uint32_t HandleMap<Handled>::allocate(Handled *element) {
	uint32_t handle = handlePool_->allocate();
	content_[handle] = element;
	return handle;
}

template<typename Handled>
Handled *HandleMap<Handled>::find(uint32_t handle) {
	typename std::map<uint32_t, Handled *>::iterator iter = content_.find(handle);
	if(iter == content_.end()) {
		return 0;
	} else {
		return (*iter).second;
	}
}

template<typename Handled>
void HandleMap<Handled>::release(uint32_t handle) {
	content_.erase(handle);
	handlePool_->release(handle);
}
