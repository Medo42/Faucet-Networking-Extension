#include <boost/integer.hpp>

#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.h>

#include <faucet/asio.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandlePool handlePool;
HandleMap<Socket> socketHandles(&handlePool);

static void handleIo() {
	ioService->poll();
	// TODO handle the potential exception/error propagated through from a handler
	// It's a code bug if this happens, so we need to try getting debug info out.
}

DLLEXPORT double tcp_connect(char *host, double port) {
	TcpSocket *socket;
	if(port>=65536 || port<0) {
		socket = TcpSocket::error("Port number out of range");
	} else {
		uint16_t intPort = (uint16_t)port;
		socket = TcpSocket::connectTo(host, intPort);
	}
	return socketHandles.allocate(socket);
}

DLLEXPORT double socket_connecting(double socketHandle) {
	handleIo();
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->isConnecting();
	} else {
		return false;
	}
}

DLLEXPORT double socket_has_error(double socketHandle) {
	handleIo();
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double socketHandle) {
	handleIo();
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

DLLEXPORT double dllStartup() {
	ioService = new boost::asio::io_service();
	return 0;
}


DLLEXPORT double dllShutdown() {
	ioService->stop();
	delete ioService;
	return 0;
}
