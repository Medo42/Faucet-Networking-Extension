#include <boost/integer.hpp>

#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>

#include <faucet/asio.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandlePool handlePool;
HandleMap<Socket> socketHandles(&handlePool);
HandleMap<CombinedTcpAcceptor> acceptorHandles(&handlePool);

static void handleIo() {
	ioService->poll();
	// TODO handle the potential exception/error propagated through from a handler
	// It's a code bug if this happens, so we need to try getting debug info out.
	// (Left unhandled like this the game will crash - but for now that's preferable to not noticing it)
}

DLLEXPORT double tcp_connect(char *host, double port) {
	uint16_t intPort;
	if(port>=65536 || port<=0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return socketHandles.allocate(TcpSocket::error(error.message()));
	} else {
		intPort = (uint16_t)port;
	}
	return socketHandles.allocate(TcpSocket::connectTo(host, intPort));
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

DLLEXPORT double tcp_listen(double port) {
	uint16_t intPort;
	if(port>=65536 || port<=0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return socketHandles.allocate(TcpSocket::error(error.message()));
	} else {
		intPort = (uint16_t)port;
	}
	return acceptorHandles.allocate(new CombinedTcpAcceptor(intPort));
}

DLLEXPORT double socket_accept(double handle) {
	CombinedTcpAcceptor *acceptor = acceptorHandles.find((uint32_t) handle);
	TcpSocket *accepted = acceptor->accept();
	if(accepted != NULL) {
		return socketHandles.allocate(accepted);
	} else {
		return -1;
	}
}

DLLEXPORT double socket_has_error(double socketHandle) {
	handleIo();
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	CombinedTcpAcceptor *acceptor = acceptorHandles.find((uint32_t) socketHandle);

	if(socket != 0) {
		return socket->hasError();
	} else if(acceptor != 0) {
		return acceptor->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double socketHandle) {
	handleIo();
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	CombinedTcpAcceptor *acceptor = acceptorHandles.find((uint32_t) socketHandle);

	if(socket != 0) {
		return socket->getErrorMessage().c_str();
	} else if(acceptor != 0) {
		return acceptor->getErrorMessage().c_str();
	} else {
		return "The socket handle is invalid.";
	}
}

DLLEXPORT double socket_destroy(double socketHandle, double immediately) {
	// TODO Handle graceful close
	Socket *socket = socketHandles.find((uint32_t) socketHandle);
	CombinedTcpAcceptor *acceptor = acceptorHandles.find((uint32_t) socketHandle);
	if(socket != 0) {
		delete socket;
		socketHandles.release((uint32_t) socketHandle);
	} else if(acceptor != 0) {
		delete acceptor;
		acceptorHandles.release((uint32_t) socketHandle);
	}
	return 0;
}

DLLEXPORT double socket_destroy_graceful(double socketHandle) {
	return socket_destroy(socketHandle, 0);
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
