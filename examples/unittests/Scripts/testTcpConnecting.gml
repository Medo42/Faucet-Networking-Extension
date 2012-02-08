var acceptor, sock1, sock2;

acceptor = tcp_listen(0);
assertFalse(socket_has_error(acceptor), "Error creating listening socket: "+socket_error(acceptor));

if(!socket_has_error(acceptor)) {
    sock1 = tcp_connect("127.0.0.1", socket_local_port(acceptor));
    sleep(100);
    sock2 = socket_accept(acceptor);
    sleep(100);
    assertTrue(sock2>0 and !socket_connecting(sock1) and !socket_has_error(sock1), "Error TCP connecting via ipv4");
    socket_destroy(sock1);
    socket_destroy(sock2);
    
    sock1 = tcp_connect("::1", socket_local_port(acceptor));
    sleep(100);
    sock2 = socket_accept(acceptor);
    sleep(100);
    assertTrue(sock2>0 and !socket_connecting(sock1) and !socket_has_error(sock1), "Error TCP connecting via ipv6");
    socket_destroy(sock1);
    socket_destroy(sock2);
}
socket_destroy(acceptor);
