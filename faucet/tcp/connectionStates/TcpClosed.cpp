#include "TcpClosed.hpp"

#include <faucet/tcp/TcpSocket.hpp>

using namespace boost::asio::ip;

TcpClosed::TcpClosed(TcpSocket &tcpSocket) :
	ConnectionState(tcpSocket), hasError(false), errorMessage() {
}

void TcpClosed::enterError(const std::string &error) {
	// Prevent an existing error message from being overwritten
	if (!hasError) {
		hasError = true;
		errorMessage = error;
	}
	enterClosed();
}

void TcpClosed::enterClosed() {
	if(getSocket().is_open()) {
		// Try to close the socket (abortive), ignoring any errors
		boost::system::error_code newError;
		boost::asio::socket_base::linger option(false, 0);
		getSocket().set_option(option, newError);
		getSocket().shutdown(tcp::socket::shutdown_both, newError);
		getSocket().close(newError);
	}
}

void TcpClosed::abort() {
	// There's nothing to abort
}
