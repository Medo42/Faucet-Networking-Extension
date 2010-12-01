#include "TcpSocket.h"

#define _WIN32_WINNT 0x501
#include <ws2tcpip.h>

TcpSocket::TcpSocket() {

}

TcpSocket::~TcpSocket() {

}

/**
 * @return 0 or SOCKET_ERROR
 */
static int setNonBlocking(SOCKET socket) {
    unsigned long nonBlocking = 1;
    return ioctlsocket(hSocket, FIONBIO, &nonBlocking);
}

TcpSocket *TcpSocket::connectTo(char *address, uint16_t port) {
    addrinfo *addrs = NULL;
    addrinfo hints;
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if(getaddrinfo(address, NULL, &hints, &addrs) != 0) {
    	// TODO Error handling
    }

    if(addrs == NULL) {
    	// TODO handle the case where no address was found
    }

    // Since we only allow numeric addresses and specify TCP, there really
    // shouldn't be more than one possible result. So we just run with the first.
    if(addrs->ai_addr->sa_family == AF_INET) {
    	((sockaddr_in *)(addrs->ai_addr))->sin_port = htons(port);
    } else if(addrs->ai_addr->sa_family == AF_INET6) {
    	((sockaddr_in6 *)(addrs->ai_addr))->sin6_port = htons(port);
    } else {
    	// TODO Error handling
    }

    SOCKET hSocket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    if(hSocket == INVALID_SOCKET) {
    	// TODO Error handling
    }

    if(setNonBlocking(hSocket) == SOCKET_ERROR) {
    	closesocket(hSocket);
    	// TODO Error handling
    }

    if(connect(hSocket, addrs->ai_addr, addrs->ai_addrlen) == SOCKET_ERROR) {
    	// TODO Error handling
    }

    freeaddrinfo(addrs);
}
