#pragma once

// Define WinXP compatibility
#define _WIN32_WINNT 0x0501
#include <boost/asio.hpp>

namespace boost {
	class thread;
}

class Asio {
public:
	static void startup();
	static boost::asio::io_service &getIoService();
	static void shutdown();
private:
	static boost::asio::io_service *ioService;
	static boost::asio::io_service::work *work;
	static boost::thread *butler;
};
