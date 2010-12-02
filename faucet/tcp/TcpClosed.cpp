#include "TcpClosed.h"

TcpClosed::TcpClosed() :
	closedByError(false),
	errorMessage() {
}

TcpClosed::TcpClosed(const std::string &message) :
	closedByError(true),
	errorMessage(message) {
}

TcpClosed::~TcpClosed() {
	// TODO Auto-generated destructor stub
}
