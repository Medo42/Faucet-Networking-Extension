#include "TcpConnecting.h"

TcpConnecting::TcpConnecting(const std::string &host, uint16_t port) :
	resolver(*ioService) {
}

TcpConnecting::~TcpConnecting() {
}
