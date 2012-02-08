buffer1 = buffer_create();
assertNonNegative(buffer1);
buffer2 = buffer_create();
assertNonNegative(buffer2);
assertFalse(buffer1==buffer2, "Same handle for different buffers");
assertEquals(0, buffer_size(buffer1), "Buffer is not empty");
assertEquals(0, buffer_bytes_left(buffer1), "Buffer has readable content");
buffer_destroy(buffer1);
buffer_destroy(buffer2);
