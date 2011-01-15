#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/udp/UdpSender.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cast.hpp>
#include <limits>

#define DLLEXPORT extern "C" __declspec(dllexport)

using boost::numeric_cast;
using boost::numeric::bad_numeric_cast;

typedef boost::shared_ptr<Buffer> BufferPtr;
typedef boost::shared_ptr<CombinedTcpAcceptor> AcceptorPtr;
HandleMap handles;

template<typename ValueType>
static ValueType convertClipped(double value) {
	if(value <= std::numeric_limits<ValueType>::min()) {
		return std::numeric_limits<ValueType>::min();
	} else if(value >= std::numeric_limits<ValueType>::max()) {
		return std::numeric_limits<ValueType>::max();
	} else {
		return static_cast<ValueType>(value);
	}
}

static void handleIo() {
	Asio::getIoService().poll();
	// TODO handle the potential exception/error propagated through from a handler
	// It's a code bug if this happens, so we need to try getting debug info out.
	// (Left unhandled like this the game will crash - but for now that's preferable to not noticing it)
}

DLLEXPORT double tcp_connect(char *host, double port) {
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t>(port);
	} catch(bad_numeric_cast &e) {
		intPort = 0;
	}
	if(intPort == 0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		return handles.allocate(TcpSocket::connectTo(host, intPort));
	}
}

DLLEXPORT double socket_connecting(double socketHandle) {
	handleIo();
	boost::shared_ptr<Socket> socket = handles.find<Socket>(socketHandle);
	if(socket) {
		return socket->isConnecting();
	} else {
		return false;
	}
}

DLLEXPORT double tcp_listen(double port) {
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t>(port);
	} catch(bad_numeric_cast &e) {
		intPort = 0;
	}
	if(intPort==0) {
		boost::system::error_code error = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		AcceptorPtr acceptor(new CombinedTcpAcceptor(intPort));
		return handles.allocate(acceptor);
	}
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
		tcpSocket->disconnect(hard);
		handles.release(handle);
		return 0;
	}

	boost::shared_ptr<CombinedTcpAcceptor> acceptor = handles.find<CombinedTcpAcceptor>(handle);
	if(acceptor) {
		handles.release(handle);		return 0;
	}

	handleIo();
	return 0;
}

/**
 * Convert a double to the target integer type and write it to the
 * given writable. Fractional numbers are rounded to the nearest integer.
 * Values outside of the target type's range will be clamped to the border
 * values of the type.
 *
 * IntType can be any integer type up to 32 bits, or int64_t, but not uint64_t.
 */
template<typename IntType>
static double writeIntValue(double handle, double value) {
	boost::shared_ptr<Writable> writable = handles.find<Writable>(handle);
	if(writable) {
		IntType converted = convertClipped<IntType>(round(value));
		writable->write(reinterpret_cast<uint8_t *>(&converted), sizeof(converted));
	}
	return 0;
}

DLLEXPORT double write_ubyte(double handle, double value) {
	return writeIntValue<uint8_t>(handle, value);
}

DLLEXPORT double write_byte(double handle, double value) {
	return writeIntValue<int8_t>(handle, value);
}

DLLEXPORT double write_ushort(double handle, double value) {
	return writeIntValue<uint16_t>(handle, value);
}

DLLEXPORT double write_short(double handle, double value) {
	return writeIntValue<int16_t>(handle, value);
}

DLLEXPORT double write_uint(double handle, double value) {
	return writeIntValue<uint32_t>(handle, value);
}

DLLEXPORT double write_int(double handle, double value) {
	return writeIntValue<int32_t>(handle, value);
}

DLLEXPORT double write_float(double handle, double value) {
	boost::shared_ptr<Writable> writable = handles.find<Writable>(handle);
	if(writable) {
		float converted = convertClipped<float>(value);
		writable->write(reinterpret_cast<uint8_t *>(&converted), sizeof(converted));
	}
	return 0;
}

DLLEXPORT double write_double(double handle, double value) {
	boost::shared_ptr<Writable> writable = handles.find<Writable>(handle);
	if(writable) {
		writable->write(reinterpret_cast<uint8_t *>(&value), sizeof(value));
	}
	return 0;
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

DLLEXPORT double tcp_receive(double socketHandle, double size) {
	handleIo();
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
		size_t intSize;
		try {
			intSize = numeric_cast<size_t>(size);
		} catch(bad_numeric_cast &e) {
			return -1;
		}
		BufferPtr result = socket->receive(intSize);
		if(result) {
			return handles.allocate(result);
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

DLLEXPORT double tcp_receive_available(double socketHandle) {
	handleIo();
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
		return handles.allocate(socket->receive());
	} else {
		BufferPtr result(new Buffer());
		return handles.allocate(result);
	}
}

DLLEXPORT double tcp_eof(double socketHandle) {
	handleIo();
	boost::shared_ptr<TcpSocket> socket = handles.find<TcpSocket>(socketHandle);
	if(socket) {
		return socket->isEof();
	} else {
		return true;
	}
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
		size_t intSize = convertClipped<size_t>(sizeLimit);
		if(intSize == 0) {
			intSize = std::numeric_limits<size_t>::max();
		}
		socket->setSendbufferLimit(intSize);
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
		buffer->setReadpos(convertClipped<size_t>(newPos));
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
		stringbuf = buffer->readString(convertClipped<size_t>(len));
		return stringbuf;
	} else {
		return "";
	}
}

/**
 * UDP
 */

DLLEXPORT double udp_send(double bufferHandle, const char *host, double port) {
	static boost::shared_ptr<UdpSender> udpSender(new UdpSender());
	BufferPtr buffer = handles.find<Buffer>(bufferHandle);

	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t>(port);
	} catch(bad_numeric_cast &e) {
		intPort = 0;
	}

	if(intPort != 0 && buffer) {
		udpSender->send(buffer, host, intPort);
	}
	return 0;
}

/**
 * Generic functions
 */

DLLEXPORT double socket_handle_io() {
	handleIo();
	return 0;
}

DLLEXPORT double debug_handles() {
	return handles.size();
}
