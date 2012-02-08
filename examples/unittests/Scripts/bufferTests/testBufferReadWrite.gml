buffer1 = buffer_create();
write_byte(buffer1, -100);
assertEquals(1, buffer_size(buffer1));
assertEquals(-100, read_byte(buffer1));

write_ubyte(buffer1, 200);
assertEquals(2, buffer_size(buffer1));
assertEquals(200, read_ubyte(buffer1));

write_short(buffer1, -20000);
assertEquals(4, buffer_size(buffer1));
assertEquals(-20000, read_short(buffer1));

write_ushort(buffer1, 40000);
assertEquals(6, buffer_size(buffer1));
assertEquals(40000, read_ushort(buffer1));

write_int(buffer1, -100000);
assertEquals(10, buffer_size(buffer1));
assertEquals(-100000, read_int(buffer1));

write_uint(buffer1, 3000000000);
assertEquals(14, buffer_size(buffer1));
assertEquals(3000000000, read_uint(buffer1));

write_float(buffer1, 123.25);
assertEquals(18, buffer_size(buffer1));
assertEquals(123.25, read_float(buffer1));

write_double(buffer1, 123.3);
assertEquals(26, buffer_size(buffer1));
assertEquals(123.3, read_double(buffer1));

write_buffer(buffer1, buffer1);
assertEquals(52, buffer_size(buffer1));
assertEquals(26, buffer_bytes_left(buffer1));
assertEquals(-100, read_byte(buffer1));
assertEquals(200, read_ubyte(buffer1));
assertEquals(-20000, read_short(buffer1));
assertEquals(40000, read_ushort(buffer1));
assertEquals(-100000, read_int(buffer1));
assertEquals(3000000000, read_uint(buffer1));
assertEquals(123.25, read_float(buffer1));
assertEquals(123.3, read_double(buffer1));

write_string(buffer1, "Hallo Welt!");
assertEquals(11, buffer_bytes_left(buffer1));
assertEquals("Hallo Welt!", read_string(buffer1, 11));
assertEquals(0, buffer_bytes_left(buffer1));

write_string(buffer1, "Hallo Welt!");
assertEquals("Hallo Welt!", read_string(buffer1, 100));
assertEquals(0, buffer_bytes_left(buffer1));

write_string(buffer1, "Hallo Welt!");
assertEquals("Hallo", read_string(buffer1, 5));
assertEquals(6, buffer_bytes_left(buffer1));
assertEquals(" Welt", read_string(buffer1, 5));
assertEquals(1, buffer_bytes_left(buffer1));

buffer_destroy(buffer1);
