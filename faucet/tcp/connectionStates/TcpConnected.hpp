#pragma once

#include "ConnectionState.hpp"

#include <memory>

class TcpConnected: public ConnectionState {
public:
	TcpConnected(TcpSocket &tcpSocket);
	virtual ~TcpConnected() {}

	void enter(bool noDelay);

	virtual void abort();

	virtual bool allowWrite() {
		return true;
	}
	virtual void startAsyncSend();
	virtual bool isEof();
	virtual bool receive(size_t ammount);
	virtual void receive();
    virtual bool setNoDelay(bool noDelay);

private:
	bool asyncSendInProgress;
	bool abortRequested;

	std::vector<uint8_t> partialReceiveBuffer;
	bool asyncReceiveInProgress;

	void nonblockReceive(size_t maxData);

	void handleSend(std::shared_ptr<TcpSocket> socket,
			const boost::system::error_code &err, size_t bytesTransferred);

	void startAsyncReceive(size_t ammount);
	void handleReceive(std::shared_ptr<TcpSocket> socket,
			const boost::system::error_code &error);
};
