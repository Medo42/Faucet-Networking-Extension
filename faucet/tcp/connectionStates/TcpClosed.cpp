#include "TcpClosed.hpp"

void TcpClosed::abort(TcpSocket &socket) {

}

bool TcpClosed::isEof(TcpSocket &socket) {
	return true;
}
