#!/usr/bin/env php
<?php
error_reporting(E_ALL);
//print_r($_SERVER);
// ini_set("trochilidae.server", "192.168.17.1");
// ini_set("trochilidae.server", "::1");
// ini_set("trochilidae.server", "localhost");
// ini_set("trochilidae.port", "30001");
echo 'trochilidae.server: ', ini_get('trochilidae.server'), PHP_EOL;
echo 'trochilidae.port: ', ini_get('trochilidae.port'), PHP_EOL;

trochilidae_set_tag("controller", "testController");
trochilidae_set_tag("action", "testAction");
trochilidae_set_tag("company", "testCompany");

echo 'trochilidae_timer_stop', PHP_EOL;
$r = trochilidae_timer_stop("db");
var_dump($r);

echo 'trochilidae_timer_start', PHP_EOL;
$r = trochilidae_timer_start("db"); var_dump($r);
usleep(100);
trochilidae_timer_stop("db");
echo 'trochilidae_timer_stop', PHP_EOL;

echo 'trochilidae_timer_start', PHP_EOL;
$r = trochilidae_timer_start("db"); var_dump($r);
usleep(200);
trochilidae_timer_stop("db");
echo 'trochilidae_timer_stop', PHP_EOL;

var_dump(trochilidae_timer_get_info());

echo 'OK', PHP_EOL;
