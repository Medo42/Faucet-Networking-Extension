#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/udp/UdpSocket.hpp>
#include <faucet/clipped_cast.hpp>
#include <faucet/GmStringBuffer.hpp>
#include <faucet/IpLookup.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/HexCodec.hpp>
#include <faucet/Base64Codec.hpp>

#include <boost/integer.hpp>
#include <boost/cast.hpp>
#include <boost/thread.hpp>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <memory>

#define DLLEXPORT extern "C" __declspec(dllexport)

// TODO: Add function fct_clear_returnbuffer()?

using boost::numeric_cast;
using boost::numeric::bad_numeric_cast;

typedef boost::lock_guard<boost::recursive_mutex> MutexLock;

HandleMap handles = HandleMap();
std::shared_ptr<UdpSocket> defaultUdpSocket = std::shared_ptr<UdpSocket>();

HexCodec hexCodec = HexCodec();
Base64Codec base64Codec = Base64Codec();

// TODO: Make the actual implementation properly thread safe instead of making the API single-threaded
boost::recursive_mutex *apiMutex = new boost::recursive_mutex;

DLLEXPORT double tcp_connect(char *host, double port) {
	MutexLock lock(*apiMutex);
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

DLLEXPORT double tcp_set_nodelay(double socketHandle, double nodelay) {
    MutexLock lock(*apiMutex);
    auto socket = handles.find<TcpSocket> (socketHandle);
    return (socket && socket->setNoDelay(nodelay >= 0.5)) ? 1 : -1;
}

DLLEXPORT double udp_bind(double port) {
	MutexLock lock(*apiMutex);
	try {
		return handles.allocate(UdpSocket::bind(numeric_cast<uint16_t> (port)));
	} catch (bad_numeric_cast &e) {
	}
	boost::system::error_code error = boost::asio::error::make_error_code(
			boost::asio::error::invalid_argument);
	return handles.allocate(UdpSocket::error(error.message()));
}

// TODO: rename to tcp_connecting in 2.0
DLLEXPORT double socket_connecting(double socketHandle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->isConnecting();
	} else {
		return false;
	}
}

DLLEXPORT double tcp_listen(double port) {
	MutexLock lock(*apiMutex);
	try {
		auto acceptor = std::make_shared<CombinedTcpAcceptor>(numeric_cast<uint16_t> (port));
		return handles.allocate(acceptor);
	} catch (bad_numeric_cast &e) {
	}
	boost::system::error_code error = boost::asio::error::make_error_code(
			boost::asio::error::invalid_argument);
	return handles.allocate(TcpSocket::error(error.message()));
}

DLLEXPORT double socket_accept(double handle) {
	MutexLock lock(*apiMutex);
	auto acceptor = handles.find<CombinedTcpAcceptor> (handle);
	if (acceptor) {
		auto accepted = acceptor->accept();

		if (accepted) {
			return handles.allocate(accepted);
		}
	}
	return -1;
}

DLLEXPORT double socket_has_error(double handle) {
	MutexLock lock(*apiMutex);
	auto fallible = handles.find<Fallible> (handle);

	if (fallible) {
		return fallible->hasError();
	} else {
		return true;
	}
}

DLLEXPORT const char *socket_error(double handle) {
	MutexLock lock(*apiMutex);
	static std::string stringbuf;
	auto fallible = handles.find<Fallible> (handle);

	if (fallible) {
		stringbuf = fallible->getErrorMessage();
		return stringbuf.c_str();
	} else {
		return "This handle is invalid.";
	}
}

// TODO: Remove in v2
DLLEXPORT double socket_handle_io() {
	return 0;
}

static void destroySocket(double handle, bool hard) {
	MutexLock lock(*apiMutex);
	auto tcpSocket = handles.find<TcpSocket> (handle);
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

	auto acceptor = handles.find<CombinedTcpAcceptor> (handle);
	if (acceptor) {
		handles.release(handle);
		return;
	}

	auto udpSocket = handles.find<UdpSocket> (handle);
	if (udpSocket) {
		if(hard) {
			udpSocket->close();
		}
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
	MutexLock lock(*apiMutex);
	auto writable = handles.find<ReadWritable> (handle);
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
	MutexLock lock(*apiMutex);
	auto writable = handles.find<ReadWritable> (handle);
	if (writable) {
		writable->writeFloat(value);
	}
	return 0;
}

DLLEXPORT double write_double(double handle, double value) {
	MutexLock lock(*apiMutex);
	std::shared_ptr<ReadWritable> writable = handles.find<ReadWritable> (
			handle);
	if (writable) {
		writable->writeDouble(value);
	}
	return 0;
}

DLLEXPORT double write_string(double handle, const char *str) {
	MutexLock lock(*apiMutex);
	auto writable = handles.find<ReadWritable> (handle);
	if (writable) {
		size_t size = strlen(str);
		writable->write(reinterpret_cast<const uint8_t *> (str), size);
	}
	return 0;
}

DLLEXPORT double write_buffer_part(double destHandle, double bufferHandle, double ammount) {
	MutexLock lock(*apiMutex);
	auto dest = handles.find<ReadWritable> (destHandle);
	auto source = handles.find<ReadWritable> (bufferHandle);

	size_t intAmmount = clipped_cast<size_t> (ammount);

	if (dest && source) {
		uint8_t tempBuffer[4096];
		intAmmount = std::min(intAmmount, source->bytesRemaining());
		size_t fullBuffers = intAmmount/4096;
		size_t lastBuffer = intAmmount%4096;
		for(size_t i=0; i<fullBuffers; i++) {
			source->read(tempBuffer, 4096);
			dest->write(tempBuffer, 4096);
		}
		source->read(tempBuffer, lastBuffer);
		dest->write(tempBuffer, lastBuffer);
		return intAmmount;
	}
	return 0;
}

DLLEXPORT double write_buffer(double destHandle, double bufferHandle) {
	MutexLock lock(*apiMutex);
	auto dest = handles.find<ReadWritable> (destHandle);
	auto srcBuffer = handles.find<Buffer> (bufferHandle);
	auto srcSocket = handles.find<Socket> (bufferHandle);

	if (dest && srcBuffer) {
		size_t oldReadPos = srcBuffer->size()-srcBuffer->bytesRemaining();
		srcBuffer->setReadpos(0);
		write_buffer_part(destHandle, bufferHandle, srcBuffer->size());
		srcBuffer->setReadpos(oldReadPos);
	} else if (dest && srcSocket) {
		dest->write(srcSocket->getReceiveBuffer().getData(),
				srcSocket->getReceiveBuffer().size());
	}
	return 0;
}

DLLEXPORT double write_hex(double destHandle, const char *hexStr) {
	MutexLock lock(*apiMutex);

    auto dest = handles.find<ReadWritable> (destHandle);

    if (!dest)
        return -1;

    return hexCodec.writeHex(hexStr, *dest);
}

DLLEXPORT double write_base64(double destHandle, const char *hexStr) {
	MutexLock lock(*apiMutex);

    auto dest = handles.find<ReadWritable> (destHandle);

    if (!dest)
        return -1;

    return base64Codec.writeBase64(hexStr, *dest);
}

DLLEXPORT double tcp_receive(double socketHandle, double size) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<TcpSocket> (socketHandle);
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
	MutexLock lock(*apiMutex);
	auto socket = handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->receive();
	} else {
		return 0;
	}
}

DLLEXPORT double tcp_eof(double socketHandle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<TcpSocket> (socketHandle);
	if (socket) {
		return socket->isEof();
	} else {
		return true;
	}
}

// TODO rename to tcp_send in 2.0
DLLEXPORT double socket_send(double socketHandle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<TcpSocket> (socketHandle);
	if (socket) {
		socket->send();
	}
	return 0;
}

DLLEXPORT double socket_sendbuffer_size(double socketHandle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (socketHandle);
	if (socket) {
		return socket->getSendbufferSize();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_receivebuffer_size(double socketHandle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (socketHandle);
	if (socket) {
		return socket->getReceivebufferSize();
	} else {
		return 0;
	}
}

DLLEXPORT double socket_sendbuffer_limit(double socketHandle, double sizeLimit) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (socketHandle);
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
	ReadWritable::setLittleEndianDefault(false);
	Asio::startup();
	return 0;
}

DLLEXPORT double dllShutdown() {
	defaultUdpSocket.reset();
	handles.releaseAll();
	Asio::shutdown();
	return 0;
}

/*********************************************
 * Buffer functions
 */

DLLEXPORT double buffer_create() {
	MutexLock lock(*apiMutex);
	auto newBuffer = std::make_shared<Buffer>();
	return handles.allocate(newBuffer);
}

DLLEXPORT double buffer_destroy(double handle) {
	MutexLock lock(*apiMutex);
	auto buffer = handles.find<Buffer> (handle);
	if (buffer) {
		handles.release(handle);
	}
	return 0;
}

DLLEXPORT double buffer_clear(double handle) {
	MutexLock lock(*apiMutex);
	auto buffer = handles.find<Buffer> (handle);
	if (buffer) {
		buffer->clear();
	}
	return 0;
}

DLLEXPORT double buffer_size(double handle) {
	MutexLock lock(*apiMutex);
	auto buffer = handles.find<Buffer> (handle);
	if (buffer) {
		return buffer->size();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_bytes_left(double handle) {
	MutexLock lock(*apiMutex);
	auto readWritable = handles.find<ReadWritable> (handle);
	if (readWritable) {
		return readWritable->bytesRemaining();
	} else {
		return 0;
	}
}

DLLEXPORT double buffer_set_readpos(double handle, double newPos) {
	MutexLock lock(*apiMutex);
	auto readWritable = handles.find<ReadWritable> (handle);
	if (readWritable) {
		readWritable->setReadpos(clipped_cast<size_t> (newPos));
	}
	return 0;
}

template<typename DesiredType>
static double readValue(double handle) {
	MutexLock lock(*apiMutex);
	auto readWritable = handles.find<ReadWritable> (handle);
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
	MutexLock lock(*apiMutex);
	auto readWritable = handles.find<ReadWritable> (handle);
	if (readWritable) {
		return replaceStringReturnBuffer(readWritable->readString(clipped_cast<size_t> (len)));
	} else {
		return "";
	}
}

DLLEXPORT const char *read_hex(double srcHandle, double dLen) {
	MutexLock lock(*apiMutex);

	auto srcBuffer = handles.find<Buffer> (srcHandle);
	auto srcSocket = handles.find<Socket> (srcHandle);

    size_t len = clipped_cast<size_t>(dLen);
	if (srcBuffer)
        return replaceStringReturnBuffer(hexCodec.readHex(*srcBuffer, len));
    else if(srcSocket)
        return replaceStringReturnBuffer(hexCodec.readHex(srcSocket->getReceiveBuffer(), len));
    else
        return "";
}

DLLEXPORT const char *read_base64(double srcHandle, double dLen) {
	MutexLock lock(*apiMutex);

	auto srcBuffer = handles.find<Buffer> (srcHandle);
	auto srcSocket = handles.find<Socket> (srcHandle);

    size_t len = clipped_cast<size_t>(dLen);
	if (srcBuffer)
        return replaceStringReturnBuffer(base64Codec.readBase64(*srcBuffer, len));
    else if(srcSocket)
        return replaceStringReturnBuffer(base64Codec.readBase64(srcSocket->getReceiveBuffer(), len));
    else
        return "";
}

// Read the entire file, appending it to the end of the buffer
DLLEXPORT double append_file_to_buffer(double handle, const char *filename) {
	MutexLock lock(*apiMutex);
	auto readWritable = handles.find<ReadWritable> (handle);
	if (!readWritable) {
		return -10;
	}

	FILE *f = fopen(filename, "rb");
	if(f == NULL) {
		return -1;	// File not found
	}
	fseek(f, 0, SEEK_END);
	long signedSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(signedSize < 0) {
		fclose(f);
		return -2;	// File too large
	}

	size_t size = signedSize;
	uint8_t *tempBuffer = (uint8_t*)malloc(size);
	if(tempBuffer == 0) {
		fclose(f);
		return -3; // Not enough memory
	}

	if(fread(tempBuffer, sizeof(uint8_t), size, f) != size) {
		free(tempBuffer);
		fclose(f);
		return -4; // Read error
	}

	readWritable->write(tempBuffer, size);

	free(tempBuffer);
	fclose(f);
	return 1;	// Success
}

// Overwrite or create the file provided with the contents of the buffer
DLLEXPORT double write_buffer_to_file(double handle, const char *filename) {
	MutexLock lock(*apiMutex);
	auto buffer = handles.find<Buffer> (handle);
	auto socket = handles.find<Socket> (handle);

	if(!buffer && !socket) {
		return -10;
	}

	Buffer &srcBuffer = buffer ? *buffer : socket->getReceiveBuffer();

	FILE *f = fopen(filename, "wb");
	if(f == NULL) {
		return -1;	// Cannot open file
	}

	if(fwrite(srcBuffer.getData(), sizeof(uint8_t), srcBuffer.size(), f) != srcBuffer.size()) {
		fclose(f);
		return -2;	// I/O-Error writing to the file
	} else {
		fclose(f);
		return 1; // Success
	}
}

/**
 * UDP
 */

static std::shared_ptr<UdpSocket> getDefaultUdpSocket() {
	if(!defaultUdpSocket) {
		defaultUdpSocket = UdpSocket::bind(0);
	}
	return defaultUdpSocket;
}

static std::shared_ptr<UdpSocket> getUdpSocketOrPrepareDefaultSocket(double handle) {
	auto buffer = handles.find<Buffer> (handle);
	if(buffer) {
		auto defaultSocket = getDefaultUdpSocket();
		defaultSocket->write(buffer->getData(), buffer->size());
		return defaultSocket;
	}

	return handles.find<UdpSocket>(handle);
}

DLLEXPORT double udp_send(double handle, const char *host, double port) {
	MutexLock lock(*apiMutex);
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}

	if (intPort == 0) {
		return false;
	}

	auto sock = getUdpSocketOrPrepareDefaultSocket(handle);
	if(sock) {
		return sock->send(host, intPort);
	}
	return false;
}

DLLEXPORT double udp_broadcast(double handle, double port) {
	MutexLock lock(*apiMutex);
	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}

	if (intPort == 0) {
		return false;
	}

	auto sock = getUdpSocketOrPrepareDefaultSocket(handle);
	if(sock) {
		return sock->broadcast(intPort);
	}
	return false;
}

DLLEXPORT double udp_receive(double handle) {
	MutexLock lock(*apiMutex);
	auto sock = handles.find<UdpSocket>(handle);
	if(sock) {
		return sock->receive();
	}
	return false;
}

/**
 * Generic functions
 */

DLLEXPORT double debug_handles() {
	MutexLock lock(*apiMutex);
	return handles.size();
}

DLLEXPORT double set_little_endian_global(double littleEndian) {
	MutexLock lock(*apiMutex);
	ReadWritable::setLittleEndianDefault(littleEndian);
	return 0;
}

DLLEXPORT double set_little_endian(double handle, double littleEndian) {
	MutexLock lock(*apiMutex);
	auto writable = handles.find<ReadWritable> (handle);
	if (writable) {
		writable->setLittleEndian(littleEndian);
	}
	return 0;
}

DLLEXPORT const char* socket_remote_ip(double handle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (handle);
	if (socket) {
		return replaceStringReturnBuffer(socket->getRemoteIp());
	}
	return "";
}

DLLEXPORT double socket_local_port(double handle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (handle);
	if (socket) {
		return socket->getLocalPort();
	}

	auto acceptor = handles.find<CombinedTcpAcceptor> (handle);
	if(acceptor) {
		return acceptor->getLocalPort();
	}

	return 0;
}

DLLEXPORT double socket_remote_port(double handle) {
	MutexLock lock(*apiMutex);
	auto socket = handles.find<Socket> (handle);
	if (socket) {
		return socket->getRemotePort();
	}
	return 0;
}

DLLEXPORT double ip_lookup_create(const char *host) {
	MutexLock lock(*apiMutex);
	return handles.allocate(IpLookup::lookup(host));
}

DLLEXPORT double ipv4_lookup_create(const char *host) {
	MutexLock lock(*apiMutex);
	return handles.allocate(IpLookup::lookup(host, boost::asio::ip::tcp::v4()));
}

DLLEXPORT double ipv6_lookup_create(const char *host) {
	MutexLock lock(*apiMutex);
	return handles.allocate(IpLookup::lookup(host, boost::asio::ip::tcp::v6()));
}

DLLEXPORT double ip_lookup_ready(double lookupHandle) {
	MutexLock lock(*apiMutex);
	auto lookup = handles.find<IpLookup>(lookupHandle);
	if(lookup) {
		return lookup->ready();
	} else {
		/*
		 * We return true here to prevent a lockup in infinite loops waiting for
		 * the lookup to finish.
		 */
		return true;
	}
}

DLLEXPORT double ip_lookup_has_next(double lookupHandle) {
	MutexLock lock(*apiMutex);
	auto lookup = handles.find<IpLookup>(lookupHandle);
	if(lookup) {
		return lookup->hasNext();
	}
	return false;
}

DLLEXPORT const char *ip_lookup_next_result(double lookupHandle) {
	MutexLock lock(*apiMutex);
	auto lookup = handles.find<IpLookup>(lookupHandle);
	if(lookup) {
		return replaceStringReturnBuffer(lookup->nextResult());
	}
	return "";
}

DLLEXPORT double ip_lookup_destroy(double lookupHandle) {
	MutexLock lock(*apiMutex);
	auto lookup = handles.find<IpLookup>(lookupHandle);
	if(lookup) {
		handles.release(lookupHandle);
	}
	return 0;
}

DLLEXPORT double ip_is_v4(const char *ip) {
	MutexLock lock(*apiMutex);
	boost::system::error_code ec;
	boost::asio::ip::address_v4::from_string(ip, ec);

	if(!ec) {
		return true;
	} else {
		return false;
	}
}

DLLEXPORT double ip_is_v6(const char *ip) {
	MutexLock lock(*apiMutex);
	boost::system::error_code ec;
	boost::asio::ip::address_v6::from_string(ip, ec);

	if(!ec) {
		return true;
	} else {
		return false;
	}
}

/**
 * Some bit manipulation functions. They are pure functions, so no locking is required.
 */
DLLEXPORT double bit_get(double source, double bitnum) {
	int8_t intbitnum = clipped_cast<int8_t>(bitnum);
	if(intbitnum < 0 || intbitnum > 63) {
		return 0;
	}

	int64_t intsource = clipped_cast<int64_t>(source);
	return (intsource & ((static_cast<int64_t>(1))<<intbitnum)) != 0 ? 1 : 0;
}

DLLEXPORT double bit_set(double source, double bitnum, double value) {
	int8_t intbitnum = clipped_cast<int8_t>(bitnum);
	if(intbitnum < 0 || intbitnum > 63) {
		return source;
	}

	int64_t intsource = clipped_cast<int64_t>(source);
	if(value>=0.5) {
		return intsource |= (static_cast<int64_t>(1) << intbitnum);
	} else {
		return intsource &= ~(static_cast<int64_t>(1) << intbitnum);
	}
}

static uint8_t build_bit(double in, uint8_t place) {
	return in>=0.5 ? (1<<place) : 0;
}

DLLEXPORT double build_ubyte(double b7, double b6, double b5, double b4, double b3, double b2, double b1, double b0) {
	return build_bit(b7,7) | build_bit(b6,6) | build_bit(b5,5) | build_bit(b4,4) | build_bit(b3,3) | build_bit(b2,2) | build_bit(b1,1) | build_bit(b0,0);
}
