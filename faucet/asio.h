#pragma once

// Define WinXP compatibility
#define _WIN32_WINNT 0x0501
#include <boost/asio.hpp>

// We only use one io_service and it's needed practically
// everywhere that asio is included.
extern boost::asio::io_service *ioService;
