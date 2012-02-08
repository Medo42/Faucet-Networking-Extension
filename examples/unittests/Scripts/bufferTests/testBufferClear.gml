buffer1 = buffer_create();
write_float(buffer1, 100.25);
buffer_clear(buffer1);
assertEquals(0, buffer_size(buffer1));
assertEquals(0, buffer_bytes_left(buffer1));
write_float(buffer1, 100.25);
assertEquals(4, buffer_size(buffer1));
assertEquals(4, buffer_bytes_left(buffer1));
read_float(buffer1);
assertEquals(4, buffer_size(buffer1));
assertEquals(0, buffer_bytes_left(buffer1));
buffer_clear(buffer1);
assertEquals(0, buffer_size(buffer1));
assertEquals(0, buffer_bytes_left(buffer1));
buffer_destroy(buffer1);

// clear invalid handle
buffer_clear(buffer1);
