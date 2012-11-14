#pragma once
#include <limits>
/**
 * Safely convert the given double to the target type, clipping to the
 * range boundaries of the target type.
 */
template<typename TargetType, typename SourceType>
static inline TargetType clipped_cast(SourceType value) {
	if(value <= std::numeric_limits<TargetType>::lowest()) {
		return std::numeric_limits<TargetType>::lowest();
	} else if(value >= std::numeric_limits<TargetType>::max()) {
		return std::numeric_limits<TargetType>::max();
	} else {
		return static_cast<TargetType>(value);
	}
}
