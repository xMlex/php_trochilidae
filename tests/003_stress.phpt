--TEST--
Check reset/flush stress and server fallback paths
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--INI--
trochilidae.server_list=localhost
--FILE--
<?php
trochilidae_set_hostname("stress-host");
echo "stress start", PHP_EOL;

for ($i = 0; $i < 25; $i++) {
    // Force collect_metrics_before_request() to consume missing host/uri.
    unset($_SERVER['HTTP_HOST'], $_SERVER['REQUEST_URI']);
    trochilidae_reset();
    trochilidae_set_tag("phase", "fallback");
    $flush1 = trochilidae_flush();
    $flush2 = trochilidae_flush();

    // Restore values and run one more cycle with normal request data.
    $_SERVER['HTTP_HOST'] = "stress.local";
    $_SERVER['REQUEST_URI'] = "/stress/" . $i;
    trochilidae_reset();
    trochilidae_set_tag("phase", "normal");
    trochilidae_timer_start("random");
    trochilidae_timer_stop("random");
    $flush3 = trochilidae_flush();

    if ($flush1 !== true || $flush2 !== true || $flush3 !== true) {
        echo "flush failed", PHP_EOL;
        break;
    }
}

echo "OK", PHP_EOL;
?>
--EXPECT--
stress start
OK
