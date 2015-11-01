var acceptor, acceptor2, sock1, sock2;

acceptor = tcp_listen(0);
assertFalse(socket_has_error(acceptor), "Error creating listening socket: "+socket_error(acceptor));
assertTrue(tcp_listening_v4(acceptor), "Acceptor not working for IPv4: " + socket_error(acceptor));
assertTrue(tcp_listening_v6(acceptor), "Acceptor not working for IPv6: " + socket_error(acceptor));

acceptor2 = tcp_listen(socket_local_port(acceptor));
assertFalse(tcp_listening_v4(acceptor2), "Acceptor listening to IPv4 port which should already be in use!");
assertFalse(tcp_listening_v6(acceptor2), "Acceptor listening to IPv6 port which should already be in use!");
assertTrue(socket_has_error(acceptor2), "Acceptor not reporting error even though it is neither listening to IPv4 nor IPv6!");
socket_destroy(acceptor2);

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
