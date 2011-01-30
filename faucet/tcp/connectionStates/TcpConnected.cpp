#include "TcpConnected.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/thread/locks.hpp>
#include <boost/bind.hpp>

using namespace boost::asio::ip;

TcpConnected::TcpConnected() :
	asyncSendInProgress(false), abortRequested(false) {
}

void TcpConnected::enter(TcpSocket &socket) {
	boost::system::error_code error;
	tcp::no_delay nodelay(true);
	if (getSocket(socket).set_option(nodelay, error)) {
		enterErrorState(socket, error.message());
		return;
	}

	startAsyncSend(socket);
}

void TcpConnected::abort(TcpSocket &socket) {
	abortRequested = true;
}

void TcpConnected::startAsyncSend(TcpSocket &socket) {
	if (!asyncSendInProgress) {
		asyncSendInProgress = true;
		getSocket(socket).async_send(
				getSendBuffer(socket).committedAsConstBufferSequence(),
				boost::bind(&TcpConnected::handleSend, this,
						socket.shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
}

bool TcpConnected::isEof(TcpSocket &socket) {
	if(partialReceiveBuffer.size()>0 || asyncReceiveInProgress) {
		return false;
	}
	try {
		uint8_t nonsenseBuffer;
		tcp::socket *asioSocket = &getSocket(socket);
		boost::asio::socket_base::non_blocking_io command(true);
		asioSocket->io_control(command);
		boost::system::error_code error;
		asioSocket->receive(boost::asio::buffer(&nonsenseBuffer, 1), asioSocket->message_peek, error);
		command = false;
		asioSocket->io_control(command);
		if(error == boost::asio::error::eof) {
			return true;
		} else if(error == boost::asio::error::would_block) {
			return false;
		} else if(error) {
			throw boost::system::system_error(error);
		} else {
			return false;
		}
	} catch(boost::system::system_error &e) {
		enterErrorState(socket, e.code().message());
		return true;
	}
}

void TcpConnected::handleSend(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error, size_t bytesTransferred) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex(*socket));
	asyncSendInProgress = false;
	if (abortRequested)
		return;

	if (!error) {
		SendBuffer *sendBuffer = &getSendBuffer(*socket);
		sendBuffer->pop(bytesTransferred);
		if (sendBuffer->committedSize() > 0) {
			startAsyncSend(*socket);
		}
	} else {
		enterErrorState(*socket, error.message());
	}
}

