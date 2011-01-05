#pragma once

#include <faucet/Handled.hpp>

#include <map>
#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>

/**
 * Provides unique handles to objects and allows access by that handle.
 * The objects must extend the "Handled" class. Objects are handled and
 * stored as shared_ptr references to allow flexible lifecycle management.
 */
class HandleMap {
	typedef boost::shared_ptr<Handled> HandledPtr;
public:
	HandleMap() {};

	/**
	 * Associate the element with a unique handle, which is returned.
	 */
	uint32_t allocate(HandledPtr element) {
		while(content_.count(nextHandle_) > 0) {
			nextHandle_++;
		}
		content_[nextHandle_] = element;
		return nextHandle_++;
	}

	/**
	 * Return the element associated with the given handle, or a NULL pointer
	 * if this Manager does not hold an object with the given handle and of
	 * the requested type.
	 *
	 * Note that you will also get NULL as result if the given handle has
	 * previously been associated with NULL.
	 */
	template<typename RequestedType>
	boost::shared_ptr<RequestedType> find(uint32_t handle) {
		std::map<uint32_t, HandledPtr>::iterator iter = content_.find(handle);
		if(iter == content_.end()) {
			return boost::shared_ptr<RequestedType>();
		} else {
			return boost::dynamic_pointer_cast<RequestedType>(iter->second);
		}
	}

	/**
	 * Convenience method for use from the Game Maker API which uses double for all numbers.
	 * Returns a NULL pointer if the double value cannot be represented as uint32_t.
	 */
	template<typename RequestedType>
	boost::shared_ptr<RequestedType> find(double handle) {
		uint32_t intHandle = (uint32_t)handle;
		if(intHandle == handle) {
			return find<RequestedType>(intHandle);
		} else {
			return boost::shared_ptr<RequestedType>();
		}
	}

	/**
	 * Release the handle-element association.
	 */
	void release(uint32_t handle) {
		content_.erase(handle);
	};

	/**
	 * Convenience method for use from the Game Maker API which uses double for all numbers.
	 * Does nothing if the double value cannot be represented as uint32_t
	 */
	void release(double handle) {
		uint32_t intHandle = (uint32_t)handle;
		if(intHandle == handle) {
			release(intHandle);
		} else {
			return;
		}
	};
private:
	uint32_t nextHandle_;
	std::map<uint32_t, HandledPtr> content_;
};
