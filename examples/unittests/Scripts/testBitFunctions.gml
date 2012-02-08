var i,j;
for(i=0; i<53; i+=1) {
    assertEquals(bit_set(0, i, 1), 1<<i);
    assertEquals(bit_set(0, i, 0), 0);
    
    for(j=0; j<53; j+=1) {
        if(i != j) {
            assertFalse(bit_get(bit_set(0, i, 1), j), string(i)+","+string(j));
        } else {
            assertTrue(bit_get(bit_set(0, i, 1), j), string(i)+","+string(j));
        }
    }
}

// Example cases from documentation
assertEquals(201, build_ubyte(1,1,0,0,1,0,0,1));
assertEquals(9, bit_set(8, 0, true));
assertEquals(-7, bit_set(-8, 0, true));
assertEquals(1, bit_get(19, 1));
