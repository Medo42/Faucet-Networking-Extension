var filename, buf1, buf2, acceptor, sock1, sock2;
filename = temp_directory+"\readwrite.test";

// Test 1: Straight reading and writing with buffers
buf1 = buffer_create();
buf2 = buffer_create();

repeat(1000)
    write_ubyte(buf1, irandom(255));

write_buffer_to_file(buf1, filename);
append_file_to_buffer(buf2, filename);

assertEquals(buffer_size(buf1), buffer_size(buf2));
repeat(buffer_size(buf1))
    assertEquals(read_ubyte(buf1), read_ubyte(buf2));
    
// Test 2: See if appending to buffer actually does just that
append_file_to_buffer(buf2, filename);
write_buffer(buf1, buf1);

assertEquals(buffer_size(buf1), buffer_size(buf2));
buffer_set_readpos(buf1, 0);
buffer_set_readpos(buf2, 0);
repeat(buffer_size(buf1))
    assertEquals(read_ubyte(buf1), read_ubyte(buf2));

// Test 3: See if reading to/from sockets works as intended
buffer_clear(buf1);
append_file_to_buffer(buf1, filename);
buffer_clear(buf2);

acceptor = tcp_listen(0);
sock1 = tcp_connect("127.0.0.1", socket_local_port(acceptor));
sock2 = 0;
while(!sock2)
    sock2 = socket_accept(acceptor);
append_file_to_buffer(sock1, filename);
socket_send(sock1);
file_delete(filename);
while(!tcp_receive(sock2, buffer_size(buf1)))
    sleep(1);
write_buffer_to_file(sock2, filename);

append_file_to_buffer(buf2, filename);
repeat(buffer_size(buf1))
    assertEquals(read_ubyte(buf1), read_ubyte(buf2));

file_delete(filename);
socket_destroy(sock1);
socket_destroy(sock2);
socket_destroy(acceptor);
buffer_destroy(buf1);
buffer_destroy(buf2);

