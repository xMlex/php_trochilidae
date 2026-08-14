--TEST--
Check multi-chunk packet with small chunk_size
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
// E2E validation with real UDP server:
//   1. php tests/udp_test_server.php --port=30009 --timeout=3 --verbose &
//   2. Set server_list to the real server address instead of 127.0.0.1:
//      ini_set("trochilidae.server_list", "127.0.0.1:30009");
//   3. Check logs for "Received chunk X/21" and "All chunks received"
//
ini_set("trochilidae.chunk_size", 120);
ini_set("trochilidae.server_list", "127.0.0.1");

// Add enough tags to force multiple chunks (payload per chunk = 120-21 = 99 bytes)
for ($i = 0; $i < 50; $i++) {
    trochilidae_set_tag("k$i", str_repeat("x", 25));
}

// Also add a timer
trochilidae_timer_start("query");
trochilidae_timer_stop("query");

trochilidae_flush();
echo "done\n";
?>
--EXPECT--
done
