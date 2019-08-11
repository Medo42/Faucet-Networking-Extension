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

write_float(buffer1, -123.25);
assertEquals(22, buffer_size(buffer1));
assertEquals(-123.25, read_float(buffer1));

write_double(buffer1, 123.3);
assertEquals(30, buffer_size(buffer1));
assertEquals(123.3, read_double(buffer1));

write_double(buffer1, -123.3);
assertEquals(38, buffer_size(buffer1));
assertEquals(-123.3, read_double(buffer1));

write_buffer(buffer1, buffer1);
assertEquals(76, buffer_size(buffer1));
assertEquals(38, buffer_bytes_left(buffer1));
assertEquals(-100, read_byte(buffer1));
assertEquals(200, read_ubyte(buffer1));
assertEquals(-20000, read_short(buffer1));
assertEquals(40000, read_ushort(buffer1));
assertEquals(-100000, read_int(buffer1));
assertEquals(3000000000, read_uint(buffer1));
assertEquals(123.25, read_float(buffer1));
assertEquals(-123.25, read_float(buffer1));
assertEquals(123.3, read_double(buffer1));
assertEquals(-123.3, read_double(buffer1));

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
read_string(buffer1, 1);

///////////////////////////////////
// Test reading delimited strings
///////////////////////////////////
assertTrue(is_real(read_delimited_string(0, "-")), "read_delimited_string should return a number if the buffer does not exist");

write_string(buffer1, "abcdef");
assertTrue(is_real(read_delimited_string(buffer1, "-")), "read_delimited_string should return a number if the delimiter is not found");
assertEquals("", read_delimited_string(buffer1, ""), "read_delimited_string with empty delimiter should return an empty string");
assertEquals("a", read_delimited_string(buffer1, "b"), "read_delimited_string should work for single-character delimiters");
assertEquals("c", read_delimited_string(buffer1, "de"), "read_delimited_string should work for multi-character delimiters");
assertEquals("", read_delimited_string(buffer1, "f"), "read_delimited_string should work for empty delimited strings");
assertEquals(0, buffer_bytes_left(buffer1), "read_delimited_string tests should not leave any data in the buffer");

assertTrue(is_real(read_cstring(0)), "read_cstring should return a number if the buffer does not exist");

write_string(buffer1, "abc");
assertTrue(is_real(read_cstring(buffer1)), "read_cstring should return a number if the buffer does not contain a 0-byte after the readpos");
write_ubyte(buffer1, 0);
write_string(buffer1, "def");
write_ubyte(buffer1, 0);
assertEquals("abc", read_cstring(buffer1), "read_cstring should read up to the first 0-byte in the string.");
assertEquals("def", read_cstring(buffer1));
assertEquals(0, buffer_bytes_left(buffer1), "read_cstring tests should not leave any data in the buffer");

///////////////////////////////
// Test hex reading / writing
///////////////////////////////
write_hex(buffer1, "466175636574");
assertEquals(6, buffer_bytes_left(buffer1));
assertEquals("Faucet", read_string(buffer1, 6));

write_string(buffer1, "Net");
assertEquals("4e6574", read_hex(buffer1, 3));

write_hex(buffer1, "0123456789abcdefABCDEF1011121314");
assertEquals(16, buffer_bytes_left(buffer1));
assertEquals("0123456789abcdefabcdef1011121314", read_hex(buffer1, 16));

write_hex(buffer1, "");
assertEquals(0, buffer_bytes_left(buffer1));

write_hex(buffer1, "012345678");
write_hex(buffer1, "01234S6789");
assertEquals(0, buffer_bytes_left(buffer1));

//////////////////////////////////
// Test base64 reading / writing
//////////////////////////////////
write_base64(buffer1, "RmF1Y2V0");
assertEquals(6, buffer_bytes_left(buffer1));
assertEquals("Faucet", read_string(buffer1, 6));

write_string(buffer1, "Net");
assertEquals("TmV0", read_base64(buffer1, 3));

// Padding
write_base64(buffer1, "UGFkZGluZw==");
assertEquals(7, buffer_bytes_left(buffer1));
assertEquals("Padding", read_string(buffer1, 7));

write_string(buffer1, "Padding");
assertEquals("UGFkZGluZw==", read_base64(buffer1, 7));

write_base64(buffer1, "UGFkZGluZ3M=");
assertEquals(8, buffer_bytes_left(buffer1));
assertEquals("Paddings", read_string(buffer1, 8));

write_string(buffer1, "Paddings");
assertEquals("UGFkZGluZ3M=", read_base64(buffer1, 8));

// Padding in the middle of a string (concatenated base64 strings)
write_base64(buffer1, "UGFkZA==aW5ncw==");
assertEquals(8, buffer_bytes_left(buffer1));
assertEquals("Paddings", read_string(buffer1, 8));

// Newline interruptions
write_base64(buffer1, "RmF"+chr(10)+"1"+chr(13)+chr(10)+"Y2V0");
assertEquals(6, buffer_bytes_left(buffer1));
assertEquals("Faucet", read_string(buffer1, 6));

// Invalid codes
write_base64(buffer1, "0==="); // Bad padding
assertEquals(0, buffer_bytes_left(buffer1));

write_base64(buffer1, "aaa#"); // Bad character
assertEquals(0, buffer_bytes_left(buffer1));

write_base64(buffer1, "");
assertEquals(0, buffer_bytes_left(buffer1));

//////////////////////////////////
// Test writing binary strings
//////////////////////////////////
write_binary_string(buffer1, "Hallo Welt!");
assertEquals(11, buffer_bytes_left(buffer1));
assertEquals("Hallo Welt!", read_string(buffer1, 11));

write_binary_string(buffer1, "Hallo"+chr(0)+"Welt!");
assertEquals(11, buffer_bytes_left(buffer1));
assertEquals("48616c6c6f0057656c7421", read_hex(buffer1, 11));

write_binary_string(buffer1, chr(0));
assertEquals(1, buffer_bytes_left(buffer1));
assertEquals("00", read_hex(buffer1, 1));

write_binary_string(buffer1, "");
assertEquals(0, buffer_bytes_left(buffer1));

//////////////////////////////////
// Test reading binary strings
//////////////////////////////////
write_binary_string(buffer1, "Hallo"+chr(0)+"Welt!");
assertEquals("Hallo"+chr(0)+"Welt!", read_binary_string(buffer1, 11));
assertEquals(0, buffer_bytes_left(buffer1));

write_binary_string(buffer1, "Hallo"+chr(0)+"Welt!");
assertEquals("Hallo"+chr(0)+"Welt", read_binary_string(buffer1, 10));
assertEquals("", read_binary_string(buffer1, 0));
assertEquals("!", read_binary_string(buffer1, 10));

/////////////////////////////////////////
// Test reading delimited binary strings
/////////////////////////////////////////

write_binary_string(buffer1, "Hallo"+chr(0)+"Welt"+chr(0)+"!");
assertEquals("Hallo", read_delimited_binary_string(buffer1, chr(0)));
assertEquals("Welt", read_delimited_binary_string(buffer1, chr(0)));
assertEquals(-1, read_delimited_binary_string(buffer1, chr(0)));
assertEquals("!", read_binary_string(buffer1, 100));

write_binary_string(buffer1, "Hallo"+chr(0)+"Welt"+chr(0)+"!");
assertEquals("Ha", read_delimited_binary_string(buffer1, "l"));
assertEquals("", read_delimited_binary_string(buffer1, "l"));
assertEquals("o"+chr(0)+"We", read_delimited_binary_string(buffer1, "l"));
assertEquals(-1, read_delimited_binary_string(buffer1, "l"));
assertEquals("t"+chr(0)+"!", read_binary_string(buffer1, 100));

write_binary_string(buffer1, "Hallo"+chr(0)+"Wolt"+chr(0)+"!");
assertEquals("Hall", read_delimited_binary_string(buffer1, "o"+chr(0)));
assertEquals(-1, read_delimited_binary_string(buffer1, "o"+chr(0)));
assertEquals("Wolt"+chr(0)+"!", read_binary_string(buffer1, 100));

buffer_destroy(buffer1);
