#include <boost/integer.hpp>

#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandlePool handlePool;
HandleMap<Socket> socketHandles(&handlePool);

DLLEXPORT double tcp_connect(char *host, double port) {
	if(port>=65536 || port<0) {
		return socketHandles.allocate(TcpSocket::error("Port number out of range"));
	}

	uint16_t intPort = (uint16_t)port;
	return socketHandles.allocate(TcpSocket::connectTo(host, intPort));
}

DLLEXPORT double socket_connecting(double socketHandle) {
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->isConnecting();
	} else {
		return false;
	}
}

DLLEXPORT double socket_has_error(double socketHandle) {
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double socketHandle) {
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->getErrorMessage().c_str();
	} else {
		return "The socket handle is invalid.";
	}
}

DLLEXPORT double socket_destroy(double socketHandle, double immediately) {
	// TODO Handle graceful close
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		delete socket;
		socketHandles.release((uint32_t) socketHandle);
	}
	return 0;
}
