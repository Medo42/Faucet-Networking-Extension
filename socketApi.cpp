#include <windows.h>
#include <map>
#include <inttypes.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

std::map<int, void*> handles;

DLLEXPORT double tcp_connect(char *host, double port) {
	uint16_t intPort = (uint16_t)port;
	if(intPort != port) {

	}
	return 0;
}

DLLEXPORT double dll_init() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	return 0;
}

DLLEXPORT double dll_finalize() {
	WSACleanup();
	return 0;
}
