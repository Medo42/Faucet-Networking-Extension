#include <boost/integer.hpp>

#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>

#include <faucet/asio.h>
#include <limits>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandleMap handles;

static void handleIo() {
	ioService->poll();
	// TODO handle the potential exception/error propagated through from a handler
	// It's a code bug if this happens, so we need to try getting debug info out.
	// (Left unhandled like this the game will crash - but for now that's preferable to not noticing it)
}

DLLEXPORT double tcp_connect(char *host, double port) {
	if(port>=65536 || port<=0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		return handles.allocate(TcpSocket::connectTo(host, (uint16_t)port));
	}
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
static double writeValue(double socketHandle, double value) {
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		ValueType converted = static_cast<ValueType>(value);
		socket->write(reinterpret_cast<uint8_t *>(&converted), sizeof(ValueType));
	}
	return 0;
}

DLLEXPORT double write_ubyte(double socketHandle, double value) {
	return writeValue<uint8_t>(socketHandle, value);
}

DLLEXPORT double write_byte(double socketHandle, double value) {
	return writeValue<int8_t>(socketHandle, value);
}

DLLEXPORT double write_ushort(double socketHandle, double value) {
	return writeValue<uint16_t>(socketHandle, value);
}

DLLEXPORT double write_short(double socketHandle, double value) {
	return writeValue<int16_t>(socketHandle, value);
}

DLLEXPORT double write_uint(double socketHandle, double value) {
	return writeValue<uint32_t>(socketHandle, value);
}

DLLEXPORT double write_int(double socketHandle, double value) {
	return writeValue<int32_t>(socketHandle, value);
}

DLLEXPORT double write_string(double socketHandle, const char *str) {
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		size_t size = strlen(str);
		socket->write(reinterpret_cast<const uint8_t *>(str), size);
	}
	return 0;
}

DLLEXPORT double socket_send(double socketHandle) {
	handleIo();
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		socket->send();
	}
	return 0;
}

DLLEXPORT double socket_sendbuffer_size(double socketHandle) {
	handleIo();
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		return socket->getSendbufferSize();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_sendbuffer_limit(double socketHandle, double sizeLimit) {
	TcpSocket *socket = handles.find<TcpSocket>((uint32_t) socketHandle);
	if(socket != 0) {
		if(sizeLimit>0 && ((size_t)sizeLimit)>0) {
			socket->setSendbufferLimit((size_t)sizeLimit);
		} else {
			socket->setSendbufferLimit(std::numeric_limits<size_t>::max());
		}
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
