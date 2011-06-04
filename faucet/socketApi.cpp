#include <faucet/Asio.hpp>
#include <faucet/HandleMap.hpp>
#include <faucet/tcp/TcpSocket.hpp>
#include <faucet/tcp/CombinedTcpAcceptor.hpp>
#include <faucet/udp/UdpSender.hpp>
#include <faucet/clipped_cast.hpp>
#include <faucet/GmStringBuffer.hpp>

#include <shared_buffers/BufferProxy.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cast.hpp>

#include <limits>

#define DLLEXPORT extern "C" __declspec(dllexport)

using boost::numeric_cast;
using boost::numeric::bad_numeric_cast;

// TODO: Actually acquire this
shb_CoreApi* shbCoreApi;

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
		return socket->getReceiveBuffer().getLength();
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

/**
 * UDP
 */

DLLEXPORT double udp_send(double bufferHandle, const char *host, double port) {
	static boost::shared_ptr<UdpSender> udpSender(new UdpSender());
	shb::BufferProxy buffer(shbCoreApi, clipped_cast<uint32_t>(bufferHandle));

	uint16_t intPort;
	try {
		intPort = numeric_cast<uint16_t> (port);
	} catch (bad_numeric_cast &e) {
		intPort = 0;
	}

	if (intPort != 0 && buffer.bufferExists()) {
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

DLLEXPORT const char* socket_remote_ip(double handle) {
	boost::shared_ptr<TcpSocket> socket =
			handles.find<TcpSocket> (handle);
	if (socket) {
		return replaceStringReturnBuffer(socket->getRemoteIp());
	}
	return "";
}
