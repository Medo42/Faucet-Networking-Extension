#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/Buffer.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <limits>

#define DLLEXPORT extern "C" __declspec(dllexport)

typedef boost::shared_ptr<Buffer> BufferPtr;
typedef boost::shared_ptr<CombinedTcpAcceptor> AcceptorPtr;
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
	boost::shared_ptr<Socket> socket = handles.find<Socket>(socketHandle);
	if(socket) {
		return socket->isConnecting();
	} else {
		return false;
	};
}

DLLEXPORT double tcp_listen(double port) {
	uint16_t intPort;
	if(port>=65536 || port<=0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		intPort = (uint16_t)port;
	}

	AcceptorPtr acceptor(new CombinedTcpAcceptor(intPort));
	return handles.allocate(acceptor);
}

DLLEXPORT double socket_accept(double handle) {
	handleIo();	AcceptorPtr acceptor = handles.find<CombinedTcpAcceptor>(handle);
	if(acceptor) {
		boost::shared_ptr<Socket> accepted = acceptor->accept();

		if(accepted) {
			return handles.allocate(accepted);
		}
	}
	return -1;
}

DLLEXPORT double socket_has_error(double handle) {
	handleIo();
	boost::shared_ptr<Fallible> fallible = handles.find<Fallible>(handle);

	if(fallible) {
		return fallible->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double handle) {
	handleIo();
	boost::shared_ptr<Fallible> fallible = handles.find<Fallible>(handle);

	if(fallible) {
		return fallible->getErrorMessage().c_str();
	} else {
		return "This handle is invalid.";
	}
}

DLLEXPORT double socket_destroy(double handle, double hard) {
	boost::shared_ptr<TcpSocket> tcpSocket = handles.find<TcpSocket>(handle);
	if(tcpSocket) {
		tcpSocket->disconnect(hard != 0);
		handles.release(handle);
		return 0;
	}

	boost::shared_ptr<CombinedTcpAcceptor> acceptor = handles.find<CombinedTcpAcceptor>(handle);
	if(acceptor) {
		handles.release(handle);		return 0;
	}

	return 0;
}

DLLEXPORT double socket_destroy_graceful(double socketHandle) {
	return socket_destroy(socketHandle, 0);
}

template<typename ValueType>
static double writeValue(double handle, double value) {
	boost::shared_ptr<Writable> writable = handles.find<Writable>(handle);
	if(writable) {
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
	boost::shared_ptr<Writable> writable = handles.find<Writable>(handle);
	if(writable) {
		size_t size = strlen(str);
		writable->write(reinterpret_cast<const uint8_t *>(str), size);
	}
	return 0;
}

DLLEXPORT double write_buffer(double destHandle, double bufferHandle) {
	boost::shared_ptr<Writable> writable = handles.find<Writable>(destHandle);
	boost::shared_ptr<Buffer> buffer = handles.find<Buffer>(bufferHandle);
	if(writable && buffer) {
		writable->write(buffer->getData(), buffer->size());
	}
	return 0;
}

DLLEXPORT double socket_send(double socketHandle) {
	handleIo();
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
		socket->send();
	}
	return 0;
}

DLLEXPORT double socket_sendbuffer_size(double socketHandle) {
	handleIo();
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
		return socket->getSendbufferSize();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_sendbuffer_limit(double socketHandle, double sizeLimit) {
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
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
	BufferPtr newBuffer(new Buffer());
	return handles.allocate(newBuffer);
}

DLLEXPORT double buffer_destroy(double handle) {
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
		handles.release(handle);
	}
	return 0;
}

DLLEXPORT double buffer_clear(double handle) {
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
		buffer->clear();
	}
	return 0;
}

DLLEXPORT double buffer_size(double handle) {
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
		return buffer->size();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_bytes_left(double handle) {
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
		return buffer->bytesRemaining();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_set_readpos(double handle, double newPos) {
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
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
	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
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

	BufferPtr buffer = handles.find<Buffer>(handle);
	if(buffer) {
		stringbuf = buffer->readString(len);
		return stringbuf;
	} else {
		return "";
	}
}
