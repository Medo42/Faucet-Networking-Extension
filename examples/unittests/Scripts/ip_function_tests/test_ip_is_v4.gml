assertTrue(ip_is_v4("127.0.0.1"), "Normal IPv4");
assertFalse(ip_is_v4("google.com"), "Hostname");
assertFalse(ip_is_v4("::1"), "IPv6");
assertTrue(ip_is_v4("255.255.255.255"), "Local broadcast");
assertFalse(ip_is_v4("255.255.255.256"), "Wrong octet value");
assertFalse(ip_is_v4("::ffff:192.0.2.128"), "IPv4-Mapped");
assertFalse(ip_is_v4(""), "Empty String");
