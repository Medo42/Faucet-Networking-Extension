buffer1 = buffer_create();
write_float(buffer1, 100.25);
buffer_destroy(buffer1);
assertEquals(0, buffer_size(buffer1));
assertEquals(0, buffer_bytes_left(buffer1));
write_float(buffer1, 100.25);
assertEquals(0, buffer_size(buffer1));
assertEquals(0, buffer_bytes_left(buffer1));

// Destroy an invalid handle
buffer_destroy(buffer1);
