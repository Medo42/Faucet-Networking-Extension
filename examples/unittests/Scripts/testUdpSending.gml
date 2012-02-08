var sock1, sock2, port;

sock1 = udp_bind(0);
sock2 = udp_bind(0);

assertFalse(socket_has_error(sock1) || socket_has_error(sock2), "Unable to bind UDP sockets.");

write_int(sock1, 1234567);
udp_send(sock1, "127.0.0.1", socket_local_port(sock2));
sleep(100);
assertTrue(udp_receive(sock2), "Sending/Receiving via UDP failed (first packet).");
assertTrue(ip_is_v4(socket_remote_ip(sock2)));
assertEquals(1234567, read_int(sock2));

write_int(sock1, 7654321);
udp_send(sock1, "::1", socket_local_port(sock2));
sleep(100);
assertTrue(udp_receive(sock2), "Sending/Receiving via UDP failed (second packet).");
assertTrue(ip_is_v6(socket_remote_ip(sock2)));
assertEquals(7654321, read_int(sock2));

socket_destroy(sock1);
socket_destroy(sock2);
