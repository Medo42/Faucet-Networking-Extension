#include "TcpConnected.hpp"

#include <faucet/tcp/TcpSocket.hpp>
#include <boost/thread/locks.hpp>
#include <boost/bind.hpp>
#include <limits>

using namespace boost::asio::ip;

TcpConnected::TcpConnected(TcpSocket &tcpSocket) :
	ConnectionState(tcpSocket), asyncSendInProgress(false),
			abortRequested(false),
			partialReceiveBuffer(boost::make_shared<std::vector<uint8_t> >()),
			asyncReceiveInProgress(false),
			delimiterEnd(0),
			scannedDelimiter() {
}

void TcpConnected::enter() {
	try {
		tcp::endpoint endpoint = getSocket().remote_endpoint();
		setRemoteIp(endpoint.address().to_string());

		// Disable Nagle's algorithm
		tcp::no_delay nodelay(true);
		getSocket().set_option(nodelay);

		// Not sure if these settings are optimal / necessary,
		// but they seemed to work well in a useless benchmark :P.
		boost::asio::socket_base::send_buffer_size sbs(0);
		getSocket().set_option(sbs);

		boost::asio::socket_base::receive_buffer_size rbs(65536);
		getSocket().set_option(rbs);
	} catch (boost::system::system_error &e) {
		enterErrorState(e.code().message());
		return;
	}

	startAsyncSend();
}

void TcpConnected::abort() {
	abortRequested = true;
}

void TcpConnected::startAsyncSend() {
	if (!asyncSendInProgress) {
		asyncSendInProgress = true;
		getSocket().async_send(
				getSendBuffer().getCommittedData(),
				boost::bind(&TcpConnected::handleSend, this,
						socket->shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}
}

void TcpConnected::handleSend(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error, size_t bytesTransferred) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());
	asyncSendInProgress = false;
	if (abortRequested)
		return;

	if (!error) {
		SendBuffer *sendBuffer = &getSendBuffer();
		sendBuffer->pop(bytesTransferred);
		if (sendBuffer->committedSize() > 0) {
			startAsyncSend();
		}
	} else {
		enterErrorState(error.message());
	}
}

bool TcpConnected::isEof() {
	if (partialReceiveBuffer->size() > 0 || asyncReceiveInProgress) {
		return false;
	}
	try {
		uint8_t nonsenseBuffer;
		tcp::socket *asioSocket = &getSocket();
		boost::asio::socket_base::non_blocking_io command(true);
		asioSocket->io_control(command);
		boost::system::error_code error;
		asioSocket->receive(boost::asio::buffer(&nonsenseBuffer, 1),
				asioSocket->message_peek, error);
		command = false;
		asioSocket->io_control(command);
		if (error == boost::asio::error::eof) {
			return true;
		} else if (error == boost::asio::error::would_block) {
			return false;
		} else if (error) {
			throw boost::system::system_error(error);
		} else {
			return false;
		}
	} catch (boost::system::system_error &e) {
		enterErrorState(e.code().message());
		return true;
	}
}

/**
 * @throws
 */
void TcpConnected::nonblockReceive(size_t maxData) {
	if (asyncReceiveInProgress) {
		return;
	}
	size_t available = getSocket().available();
	size_t readAmmount = std::min(maxData, available);
	size_t recvBufferEndIndex = partialReceiveBuffer->size();
	partialReceiveBuffer->insert(partialReceiveBuffer->end(), readAmmount, 0);
	boost::asio::read(
			getSocket(),
			boost::asio::buffer(
					partialReceiveBuffer->data() + recvBufferEndIndex,
					readAmmount));
}

bool TcpConnected::receive(size_t ammount) {
	if (asyncReceiveInProgress) {
		return false;
	}

	if (partialReceiveBuffer->size() < ammount) {
		// Try to satisfy the request with a nonblocking read
		try {
			nonblockReceive(ammount - partialReceiveBuffer->size());
		} catch (boost::system::system_error &e) {
			enterErrorState(e.code().message());
			return false;
		}
	}

	if (partialReceiveBuffer->size() == ammount) {
		getReceiveBuffer().replaceContent(partialReceiveBuffer);
		partialReceiveBuffer = boost::make_shared<std::vector<uint8_t> >();
		delimiterEnd = 0;
		return true;
	} else if (partialReceiveBuffer->size() > ammount) {
		getReceiveBuffer().write(partialReceiveBuffer->data(), ammount);
		partialReceiveBuffer->erase(partialReceiveBuffer->begin(),
				partialReceiveBuffer->begin() + ammount);
		delimiterEnd = 0;
		return true;
	} else {
		size_t remaining = ammount - partialReceiveBuffer->size();
		startAsyncReceive(remaining);
		return false;
	}
}

void TcpConnected::receive() {
	if (asyncReceiveInProgress) {
		return;
	}

	try {
		nonblockReceive(std::numeric_limits<size_t>::max());
		getReceiveBuffer().replaceContent(partialReceiveBuffer);
		partialReceiveBuffer = boost::make_shared<std::vector<uint8_t> >();
		delimiterEnd = 0;
	} catch (boost::system::system_error &e) {
		enterErrorState(e.code().message());
	}
}

int TcpConnected::receiveDelimited(std::string delimiter, size_t maxSize) {
	if (asyncReceiveInProgress) {
		return 0;
	}

	for(std::vector<uint8_t>::iterator iter = partialReceiveBuffer->begin(); iter != partialReceiveBuffer->end(); ++iter) {
		for(std::string::iterator strIter = delimiter.begin(); strIter != delimiter.end(); strIter++) {

		}
	}

	if(delimiterEnd > 0) {
		getReceiveBuffer().write(partialReceiveBuffer->data(), delimiterEnd-scannedDelimiter.size());
		partialReceiveBuffer->erase(partialReceiveBuffer->begin(),
				partialReceiveBuffer->begin() + delimiterEnd);
		delimiterEnd = 0;
		return true;
	}



	try {
		nonblockReceive(std::numeric_limits<size_t>::max());
		getReceiveBuffer().replaceContent(partialReceiveBuffer);
		partialReceiveBuffer = boost::make_shared<std::vector<uint8_t> >();
	} catch (boost::system::system_error &e) {
		enterErrorState(e.code().message());
	}
}

void TcpConnected::startAsyncReceive(size_t ammount) {
	if (!asyncReceiveInProgress) {
		asyncReceiveInProgress = true;

		size_t recvBufferEndIndex = partialReceiveBuffer->size();
		partialReceiveBuffer->insert(partialReceiveBuffer->end(), ammount, 0);
		boost::asio::async_read(
				getSocket(),
				boost::asio::buffer(
						partialReceiveBuffer->data() + recvBufferEndIndex,
						ammount),
				boost::bind(&TcpConnected::handleReceive, this,
						socket->shared_from_this(),
						boost::asio::placeholders::error));
	}
}

void TcpConnected::handleReceive(boost::shared_ptr<TcpSocket> socket,
		const boost::system::error_code &error) {
	boost::lock_guard<boost::recursive_mutex> guard(getCommonMutex());
	asyncReceiveInProgress = false;
	if (error) {
		enterErrorState(error.message());
	}
}
