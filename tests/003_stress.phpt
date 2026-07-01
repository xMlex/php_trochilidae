--TEST--
Check hostname setting and reset/flush functionality
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
$hostname = "test-host";
trochilidae_set_hostname('test');
trochilidae_set_hostname($hostname);
echo "Hostname set successfully", PHP_EOL;

for ($i = 0; $i < 10; $i++) {
    trochilidae_reset();
    trochilidae_timer_start("random");
    trochilidae_timer_stop("random");
    trochilidae_flush();
}

echo 'OK', PHP_EOL;
?>
--EXPECT--
Hostname set successfully
OK
