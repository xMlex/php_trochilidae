--TEST--
trochilidae flush 20x+ loop (memory exhaustion regression - original)
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
ini_set('memory_limit', '128M');

for ($i = 0; $i < 20; $i++) {
    for ($j = 0; $j < 25; $j++) {
        trochilidae_flush();
    }
}

echo "OK, mem: " . (memory_get_peak_usage(true) / 1024 / 1024) . " Mb" . PHP_EOL;
--EXPECT--
OK, mem: 2 Mb