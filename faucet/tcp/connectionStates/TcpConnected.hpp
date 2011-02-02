#pragma once

#include "ConnectionState.hpp"

#include <boost/shared_ptr.hpp>

class TcpConnected: public ConnectionState {
public:
	TcpConnected(TcpSocket &tcpSocket);
	void enter();

	virtual void abort();

	virtual bool allowWrite() {
		return true;
	}
	virtual void startAsyncSend();
	virtual bool isEof();
	virtual bool receive(size_t ammount);
	virtual void receive();

private:
	bool asyncSendInProgress;
	bool abortRequested;

	std::vector<uint8_t> partialReceiveBuffer;
	bool asyncReceiveInProgress;

	void nonblockReceive(size_t maxData);

	void handleSend(boost::shared_ptr<TcpSocket> socket,
			const boost::system::error_code &err, size_t bytesTransferred);

	void startAsyncReceive(size_t ammount);
	void handleReceive(boost::shared_ptr<TcpSocket> socket,
			const boost::system::error_code &error);
};
