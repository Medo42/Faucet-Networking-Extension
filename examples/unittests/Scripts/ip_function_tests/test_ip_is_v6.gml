assertTrue(ip_is_v6("0000:0000:0000:0000:0000:0000:0000:0001"), "Normal ipv6");
assertTrue(ip_is_v6("::1"), "Shortened form");
assertFalse(ip_is_v6("google.com"), "Hostname");
assertFalse(ip_is_v6("127.0.0.1"), "IPv6");
assertTrue(ip_is_v6("::ffff:192.0.2.128"), "IPv4-Mapped");
assertFalse(ip_is_v6("::10000"), "Wrong value");
assertFalse(ip_is_v6(""), "Empty String");
