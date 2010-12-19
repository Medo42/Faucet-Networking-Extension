#include <boost/integer.hpp>

#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>

#include <faucet/asio.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandlePool handlePool;
HandleMap handles(&handlePool);

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
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		intPort = (uint16_t)port;
	}
	return handles.allocate(TcpSocket::connectTo(host, intPort));
}

DLLEXPORT double socket_connecting(double socketHandle) {
	handleIo();
	Socket *socket = handles.find<Socket>((uint32_t) socketHandle);
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
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		intPort = (uint16_t)port;
	}
	return handles.allocate(new CombinedTcpAcceptor(intPort));
}

DLLEXPORT double socket_accept(double handle) {
	CombinedTcpAcceptor *acceptor = handles.find<CombinedTcpAcceptor>((uint32_t) handle);
	TcpSocket *accepted = acceptor->accept();
	if(accepted != NULL) {
		return handles.allocate(accepted);
	} else {
		return -1;
	}
}

DLLEXPORT double socket_has_error(double handle) {
	handleIo();
	Fallible *fallible = handles.find<Fallible>((uint32_t) handle);

	if(fallible != 0) {
		return fallible->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double handle) {
	handleIo();
	Fallible *fallible = handles.find<Fallible>((uint32_t) handle);

	if(fallible != 0) {
		return fallible->getErrorMessage().c_str();
	} else {
		return "This handle is invalid.";
	}
}

DLLEXPORT double socket_destroy(double handle, double immediately) {
	// TODO Handle graceful close
	Handled *handled = handles.find<Handled>((uint32_t) handle);
	if(handled != 0) {
		delete handled;
		handles.release((uint32_t) handle);
	}
	return 0;
}

DLLEXPORT double socket_destroy_graceful(double socketHandle) {
	return socket_destroy(socketHandle, 0);
}

template<typename ValueType>
static void writeValue(double socketHandle, double value) {
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		socket->writePod(static_cast<ValueType>(value));
	}
}

DLLEXPORT void write_ubyte(double socketHandle, double value) {
	writeValue<uint8_t>(socketHandle, value);
}

DLLEXPORT void write_byte(double socketHandle, double value) {
	writeValue<int8_t>(socketHandle, value);
}

DLLEXPORT void write_ushort(double socketHandle, double value) {
	writeValue<uint16_t>(socketHandle, value);
}

DLLEXPORT void write_short(double socketHandle, double value) {
	writeValue<int16_t>(socketHandle, value);
}

DLLEXPORT void write_uint(double socketHandle, double value) {
	writeValue<uint32_t>(socketHandle, value);
}

DLLEXPORT void write_int(double socketHandle, double value) {
	writeValue<int32_t>(socketHandle, value);
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
