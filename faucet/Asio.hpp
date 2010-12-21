#pragma once

// Define WinXP compatibility
#define _WIN32_WINNT 0x0501
#include <boost/asio.hpp>

class Asio {
public:
	static boost::asio::io_service &getIoService() {
		if(ioService == 0) {
			ioService = new boost::asio::io_service();
		}
		return *ioService;
	}

	static void destroyIoService() {
		delete ioService;
		ioService = 0;
	}

private:
	static boost::asio::io_service *ioService;
};
