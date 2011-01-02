#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/Buffer.hpp>

#include <boost/integer.hpp>
#include <limits>

#define DLLEXPORT extern "C" __declspec(dllexport)

HandleMap handles;

static void handleIo() {
	Asio::getIoService().poll();
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
static double writeValue(double handle, double value) {
	Writable *writable = handles.find<Writable>((uint32_t) handle);
	if(writable != 0) {
		ValueType converted = static_cast<ValueType>(value);
		writable->write(reinterpret_cast<uint8_t *>(&converted), sizeof(ValueType));
	}
	return 0;
}

DLLEXPORT double write_ubyte(double handle, double value) {
	return writeValue<uint8_t>(handle, value);
}

DLLEXPORT double write_byte(double handle, double value) {
	return writeValue<int8_t>(handle, value);
}

DLLEXPORT double write_ushort(double handle, double value) {
	return writeValue<uint16_t>(handle, value);
}

DLLEXPORT double write_short(double handle, double value) {
	return writeValue<int16_t>(handle, value);
}

DLLEXPORT double write_uint(double handle, double value) {
	return writeValue<uint32_t>(handle, value);
}

DLLEXPORT double write_int(double handle, double value) {
	return writeValue<int32_t>(handle, value);
}

DLLEXPORT double write_float(double handle, double value) {
	return writeValue<float>(handle, value);
}

DLLEXPORT double write_double(double handle, double value) {
	return writeValue<double>(handle, value);
}

DLLEXPORT double write_string(double handle, const char *str) {
	Writable *writable = handles.find<Writable>((uint32_t) handle);
	if(writable != 0) {
		size_t size = strlen(str);
		writable->write(reinterpret_cast<const uint8_t *>(str), size);
	}
	return 0;
}

DLLEXPORT double write_buffer(double destHandle, double bufferHandle) {
	Writable *writable = handles.find<Writable>((uint32_t) destHandle);
	Buffer *buffer = handles.find<Buffer>((uint32_t) bufferHandle);
	if(writable != 0 && buffer != 0) {
		writable->write(buffer->getData(), buffer->size());
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

DLLEXPORT double dllShutdown() {
	Asio::getIoService().stop();
	Asio::destroyIoService();
	return 0;
}

/*********************************************
 * Buffer functions
 */

DLLEXPORT double buffer_create() {
	return handles.allocate(new Buffer());
}

DLLEXPORT double buffer_destroy(double handle) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		handles.release((uint32_t) handle);
		delete buffer;
	}
	return 0;
}

DLLEXPORT double buffer_clear(double handle) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		buffer->clear();
	}
	return 0;
}

DLLEXPORT double buffer_size(double handle) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		return buffer->size();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_bytes_left(double handle) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		return buffer->bytesRemaining();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_set_readpos(double handle, double newPos) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		size_t intNewPos;
		if(newPos<0) {
			intNewPos = 0;
		} else if(newPos > std::numeric_limits<size_t>::max()) {
			intNewPos = std::numeric_limits<size_t>::max();
		} else {
			intNewPos = static_cast<size_t>(newPos);
		}
		buffer->setReadpos(intNewPos);
	}
	return 0;
}

template <typename DesiredType>
static double readValue(double handle) {
	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		DesiredType value;
		buffer->read(reinterpret_cast<uint8_t *>(&value), sizeof(DesiredType));
		return static_cast<double>(value);
	} else {
		return 0;
	}
}

DLLEXPORT double read_ubyte(double handle) {
	return readValue<uint8_t>(handle);
}

DLLEXPORT double read_byte(double handle) {
	return readValue<int8_t>(handle);
}

DLLEXPORT double read_ushort(double handle) {
	return readValue<uint16_t>(handle);
}

DLLEXPORT double read_short(double handle) {
	return readValue<int16_t>(handle);
}

DLLEXPORT double read_uint(double handle) {
	return readValue<uint32_t>(handle);
}

DLLEXPORT double read_int(double handle) {
	return readValue<int32_t>(handle);
}

DLLEXPORT double read_float(double handle) {
	return readValue<float>(handle);
}

DLLEXPORT double read_double(double handle) {
	return readValue<double>(handle);
}

DLLEXPORT const char *read_string(double handle, double len) {
	static char *stringbuf = 0;
	delete stringbuf;

	Buffer *buffer = handles.find<Buffer>((uint32_t) handle);
	if(buffer != 0) {
		stringbuf = buffer->readString(len);
		return stringbuf;
	} else {
		return "";
	}
}
