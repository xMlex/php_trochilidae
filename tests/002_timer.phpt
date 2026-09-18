--TEST--
Check timer functionality
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--INI--
trochilidae.server_list=localhost
--FILE--
<?php
trochilidae_set_tag("controller", "testController");
trochilidae_set_tag("action", "testAction");
trochilidae_set_tag("company", "testCompany");

echo 'trochilidae_timer_start', PHP_EOL;
$r1 = trochilidae_timer_start("db");
usleep(100);
trochilidae_timer_stop("db");
var_dump($r1);

echo 'trochilidae_timer_start', PHP_EOL;
$r2 = trochilidae_timer_start("db");
usleep(200);
trochilidae_timer_stop("db");
var_dump($r2);

$info = trochilidae_timer_get_info();
var_dump(is_array($info));
?>
--EXPECTF--
trochilidae_timer_start
bool(true)
trochilidae_timer_start
bool(true)
bool(true)
