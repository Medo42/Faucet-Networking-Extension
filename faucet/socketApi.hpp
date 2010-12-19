#pragma once

#include <boost/integer.hpp>

#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT double tcp_connect(char *host, double port);

DLLEXPORT double socket_connecting(double socketHandle);

DLLEXPORT double tcp_listen(double port);

DLLEXPORT double socket_accept(double handle);

DLLEXPORT double socket_has_error(double socketHandle);

DLLEXPORT const char *socket_error(double socketHandle);

DLLEXPORT double socket_destroy(double socketHandle, double immediately);

DLLEXPORT double socket_destroy_graceful(double socketHandle);

DLLEXPORT double dllStartup();

DLLEXPORT double dllShutdown();
