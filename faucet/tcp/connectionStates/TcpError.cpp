#include "TcpError.hpp"

#include <faucet/tcp/TcpSocket.hpp>

TcpError::TcpError() {}

void TcpError::enter(TcpSocket &socket, const std::string &error) {
	errorMessage = error;
	// TODO the whole teardown shebang
}

void TcpError::abort(TcpSocket &socket) {
	// There's no way to abort, we're already in error state.
}

bool TcpError::isEof(TcpSocket &socket) {
	return true;
}
