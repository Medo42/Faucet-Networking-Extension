buffer1 = buffer_create();
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");
buffer_set_readpos(buffer1, -10);
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");
buffer_set_readpos(buffer1, 10);
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");

write_double(buffer1, 100);
buffer_set_readpos(buffer1, -10);
assertEquals(8, buffer_bytes_left(buffer1), "Wrong number of bytes left");
buffer_set_readpos(buffer1, 10);
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");
buffer_set_readpos(buffer1, 4);
assertEquals(4, buffer_bytes_left(buffer1), "Wrong number of bytes left");
buffer_set_readpos(buffer1, 8);
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");

buffer_destroy(buffer1);
buffer_set_readpos(buffer1, 8);
assertEquals(0, buffer_bytes_left(buffer1), "Wrong number of bytes left");
