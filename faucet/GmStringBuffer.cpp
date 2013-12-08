#include "GmStringBuffer.hpp"

#define BOOST_THREAD_USE_LIB
#include<boost/thread.hpp>

boost::thread_specific_ptr<std::string> stringReturnPtr;

const char *replaceStringReturnBuffer(const std::string &str) {
	if(!stringReturnPtr.get())
		stringReturnPtr.reset(new std::string());
	(*stringReturnPtr).assign(str);
	return (*stringReturnPtr).c_str();
}
