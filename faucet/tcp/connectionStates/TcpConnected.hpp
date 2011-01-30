#pragma once

#include "ConnectionState.hpp"

#include <boost/shared_ptr.hpp>

class TcpConnected: public ConnectionState {
public:
	TcpConnected();
	void enter(TcpSocket &socket);

	virtual void abort(TcpSocket &socket);

	virtual bool allowWrite() {
		return true;
	}
	virtual void startAsyncSend(TcpSocket &socket);
	virtual bool isEof(TcpSocket &socket);

private:
	bool asyncSendInProgress;
	bool abortRequested;

	std::vector<uint8_t> partialReceiveBuffer;
	bool asyncReceiveInProgress;

	void handleSend(boost::shared_ptr<TcpSocket> socket,
			const boost::system::error_code &err, size_t bytesTransferred);
};
