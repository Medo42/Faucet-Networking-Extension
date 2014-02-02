var sock;

assertFalse(tcp_set_nodelay(0, false));

sock = tcp_listen(0);
assertFalse(tcp_set_nodelay(sock, false), "tcp_set_nodelay shouldn't work with invalid handle");
socket_destroy(sock);

sock = udp_bind(0);
assertFalse(tcp_set_nodelay(sock, false), "tcp_set_nodelay shouldn't work with UDP socket");
socket_destroy(sock);

sock = tcp_connect("ganggarrison.com", 80);
assertTrue(tcp_set_nodelay(sock, false), "tcp_set_nodelay should work on a connecting socket");

while(socket_connecting(sock))
    sleep(10);

if(!socket_has_error(sock))
    assertTrue(tcp_set_nodelay(sock, true), "tcp_set_nodelay should work on a connected socket");
else
    show_message("Failed to connect to ganggarrison.com for tcp_nodelay testing.");
    
socket_destroy(sock);
