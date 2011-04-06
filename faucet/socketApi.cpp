#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/udp/UdpSender.hpp>
#include <faucet/clipped_cast.hpp>
#include <faucet/GmStringBuffer.hpp>

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

DLLEXPORT double tcp_connect(char *host, double port) {
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}
	if (intPort == 0) {
		boost::system::error_code error = boost::asio::error::make_error_code(
				boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		return handles.allocate(TcpSocket::connectTo(host, intPort));
	}
}

DLLEXPORT double socket_connecting(double socketHandle) {
	boost::shared_ptr<Socket> socket = handles.find<Socket> (socketHandle);
	if (socket) {
		return socket->isConnecting();
	} else {
		return false;
	}
}

DLLEXPORT double tcp_listen(double port) {
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}
	if (intPort == 0) {
		boost::system::error_code error = boost::asio::error::make_error_code(
				boost::asio::error::invalid_argument);
		return handles.allocate(TcpSocket::error(error.message()));
	} else {
		AcceptorPtr acceptor(new CombinedTcpAcceptor(intPort));
		return handles.allocate(acceptor);
	}
}

DLLEXPORT double socket_accept(double handle) {
	AcceptorPtr acceptor = handles.find<CombinedTcpAcceptor> (handle);
	if (acceptor) {
		boost::shared_ptr<Socket> accepted = acceptor->accept();

		if (accepted) {
			return handles.allocate(accepted);
		}
	}
	return -1;
}

DLLEXPORT double socket_has_error(double handle) {
	boost::shared_ptr<Fallible> fallible = handles.find<Fallible> (handle);

	if (fallible) {
		return fallible->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double handle) {
	static std::string stringbuf;
	boost::shared_ptr<Fallible> fallible = handles.find<Fallible> (handle);

	if (fallible) {
		stringbuf = fallible->getErrorMessage();
		return stringbuf.c_str();
	} else {
		return "This handle is invalid.";
	}
}

DLLEXPORT double socket_handle_io() {
	// TODO: Function left in for compatibility. Remove in v2
	return 0;
}

static void destroySocket(double handle, bool hard) {
	boost::shared_ptr<TcpSocket> tcpSocket = handles.find<TcpSocket> (handle);
	if (tcpSocket) {
		if (hard) {
			tcpSocket->disconnectAbortive();
		} else {
			/*
			 * For a graceful disconnect, it's enough to just let the socket
			 * object die. However, we want to make sure that everything
			 * left in the send buffer is committed in case the library user forgot.
			 */
			tcpSocket->send();
		}
		handles.release(handle);
		return;
	}

	boost::shared_ptr<CombinedTcpAcceptor> acceptor = handles.find<
			CombinedTcpAcceptor> (handle);
	if (acceptor) {
		handles.release(handle);
		return;
	}
}

DLLEXPORT double socket_destroy(double handle) {
	destroySocket(handle, false);
	return 0;
}

DLLEXPORT double socket_destroy_abortive(double handle) {
	destroySocket(handle, true);
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
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		writable->writeIntValue<IntType> (value);
	}
	return 0;
}

DLLEXPORT double write_ubyte(double handle, double value) {
	return writeIntValue<uint8_t> (handle, value);
}

DLLEXPORT double write_byte(double handle, double value) {
	return writeIntValue<int8_t> (handle, value);
}

DLLEXPORT double write_ushort(double handle, double value) {
	return writeIntValue<uint16_t> (handle, value);
}

DLLEXPORT double write_short(double handle, double value) {
	return writeIntValue<int16_t> (handle, value);
}

DLLEXPORT double write_uint(double handle, double value) {
	return writeIntValue<uint32_t> (handle, value);
}

DLLEXPORT double write_int(double handle, double value) {
	return writeIntValue<int32_t> (handle, value);
}

DLLEXPORT double write_float(double handle, double value) {
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		writable->writeFloat(value);
	}
	return 0;
}

DLLEXPORT double write_double(double handle, double value) {
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		writable->writeDouble(value);
	}
	return 0;
}

DLLEXPORT double write_string(double handle, const char *str) {
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		size_t size = strlen(str);
		writable->write(reinterpret_cast<const uint8_t *> (str), size);
	}
	return 0;
}

DLLEXPORT double write_buffer(double destHandle, double bufferHandle) {
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			destHandle);
	boost::shared_ptr<Buffer> buffer = handles.find<Buffer> (bufferHandle);
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (bufferHandle);

	if (writable && buffer) {
		writable->write(buffer->getData(), buffer->size());
	} else if (writable && socket) {
		writable->write(socket->getReceiveBuffer().getData(),
				socket->getReceiveBuffer().size());
	}
	return 0;
}

DLLEXPORT double tcp_receive(double socketHandle, double size) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		size_t intSize;
		try {
			intSize = numeric_cast<size_t> (size);
		} catch (bad_numeric_cast &e) {
			return false;
		}
		return socket->receive(intSize);
	} else {
		return false;
	}
}

DLLEXPORT double tcp_receive_available(double socketHandle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->receive();
	} else {
		return 0;
	}
}

DLLEXPORT double tcp_eof(double socketHandle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->isEof();
	} else {
		return true;
	}
}

DLLEXPORT double socket_send(double socketHandle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		socket->send();
	}
	return 0;
}

DLLEXPORT double socket_sendbuffer_size(double socketHandle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->getSendbufferSize();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_receivebuffer_size(double socketHandle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->getReceiveBuffer().size();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_sendbuffer_limit(double socketHandle, double sizeLimit) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (socketHandle);
	if (socket) {
		size_t intSize = clipped_cast<size_t> (sizeLimit);
		if (intSize == 0) {
			intSize = std::numeric_limits<size_t>::max();
		}
		socket->setSendbufferLimit(intSize);
	}
	return 0;
}

DLLEXPORT double dllStartup() {
	Asio::startup();
	return 0;
}

DLLEXPORT double dllShutdown() {
	handles.releaseAll();
	Asio::shutdown();
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
	BufferPtr buffer = handles.find<Buffer> (handle);
	if (buffer) {
		handles.release(handle);
	}
	return 0;
}

DLLEXPORT double buffer_clear(double handle) {
	BufferPtr buffer = handles.find<Buffer> (handle);
	if (buffer) {
		buffer->clear();
	}
	return 0;
}

DLLEXPORT double buffer_size(double handle) {
	BufferPtr buffer = handles.find<Buffer> (handle);
	if (buffer) {
		return buffer->size();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_bytes_left(double handle) {
	BufferPtr buffer = handles.find<Buffer> (handle);
	if (buffer) {
		return buffer->bytesRemaining();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_set_readpos(double handle, double newPos) {
	BufferPtr buffer = handles.find<Buffer> (handle);
	if (buffer) {
		buffer->setReadpos(clipped_cast<size_t> (newPos));
	}
	return 0;
}

template<typename DesiredType>
static double readValue(double handle) {
	boost::shared_ptr<ReadWritable> readWritable = handles.find<ReadWritable> (
			handle);
	if (readWritable) {
		return readWritable->readValue<DesiredType> ();
	} else {
		return 0;
	}
}

DLLEXPORT double read_ubyte(double handle) {
	return readValue<uint8_t> (handle);
}

DLLEXPORT double read_byte(double handle) {
	return readValue<int8_t> (handle);
}

DLLEXPORT double read_ushort(double handle) {
	return readValue<uint16_t> (handle);
}

DLLEXPORT double read_short(double handle) {
	return readValue<int16_t> (handle);
}

DLLEXPORT double read_uint(double handle) {
	return readValue<uint32_t> (handle);
}

DLLEXPORT double read_int(double handle) {
	return readValue<int32_t> (handle);
}

DLLEXPORT double read_float(double handle) {
	return readValue<float> (handle);
}

DLLEXPORT double read_double(double handle) {
	return readValue<double> (handle);
}

DLLEXPORT const char *read_string(double handle, double len) {
	boost::shared_ptr<ReadWritable> readWritable = handles.find<ReadWritable> (
			handle);
	if (readWritable) {
		std::string str = readWritable->readString(clipped_cast<size_t> (len));
		return replaceStringReturnBuffer(str);
	} else {
		return "";
	}
}

/**
 * UDP
 */

DLLEXPORT double udp_send(double bufferHandle, const char *host, double port) {
	static boost::shared_ptr<UdpSender> udpSender(new UdpSender());
	BufferPtr buffer = handles.find<Buffer> (bufferHandle);

	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}

	if (intPort != 0 && buffer) {
		udpSender->send(buffer, host, intPort);
	}
	return 0;
}

/**
 * Generic functions
 */

DLLEXPORT double debug_handles() {
	return handles.size();
}

DLLEXPORT double set_little_endian_global(double littleEndian) {
	ReadWritable::setLittleEndianDefault(littleEndian);
	return 0;
}

DLLEXPORT double set_little_endian(double handle, double littleEndian) {
	boost::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		writable->setLittleEndian(littleEndian);
	}
	return 0;
}

DLLEXPORT const char* socket_remote_ip(double handle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (handle);
	if (socket) {
		return replaceStringReturnBuffer(socket->getRemoteIp());
	}
	return "";
}
