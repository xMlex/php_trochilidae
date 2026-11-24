--TEST--
trochilidae numeric tag and timer keys are kept as strings
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) die("skip extension not loaded"); ?>
--FILE--
<?php
trochilidae_set_tag("123", "v");
trochilidae_timer_start("7");
trochilidae_timer_stop("7");
trochilidae_timer_start("7");
trochilidae_timer_stop("7");

$info = trochilidae_timer_get_info()["timers"];
printf("timers=%d start=%d stop=%d\n", count($info), $info[0]["startCount"], $info[0]["stopCount"]);

trochilidae_flush();
echo "OK\n";
?>
--EXPECTF--
timers=1 start=2 stop=2
OK
