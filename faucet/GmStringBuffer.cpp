#include "GmStringBuffer.hpp"
#include<boost/thread.hpp>

boost::thread_specific_ptr<std::string> stringReturnPtr;

std::string &getStringBuffer()
{
    std::string *buf = stringReturnPtr.get();
    if(!buf)
    {
        buf = new std::string;
        stringReturnPtr.reset(buf);
    }
    return *buf;
}

const char *replaceStringReturnBuffer(const std::string &str)
{
    std::string &buf = getStringBuffer();
	buf = str;
	return buf.c_str();
}

const char *replaceStringReturnBuffer(std::string &&str)
{
    std::string &buf = getStringBuffer();
	buf = std::move(str);
	return buf.c_str();
}
