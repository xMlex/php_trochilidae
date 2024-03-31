#!/usr/bin/env php
<?php
error_reporting(E_ALL);
//print_r($_SERVER);
echo 'trochilidae.server_list: ', ini_get('trochilidae.server_list'), PHP_EOL;
ini_set('trochilidae.server_list', 'localhost,192.168.1.2:999');

trochilidae_set_tag("controller", "testController");
trochilidae_set_tag("action", "testAction");
trochilidae_set_tag("company", "testCompany");

echo 'trochilidae_timer_stop', PHP_EOL;
$r = trochilidae_timer_stop("db");
var_dump($r);

echo 'trochilidae_timer_start', PHP_EOL;
$r = trochilidae_timer_start("db");
usleep(100);
trochilidae_timer_stop("db");
var_dump($r);
echo 'trochilidae_timer_stop', PHP_EOL;

echo 'trochilidae_timer_start', PHP_EOL;
$r = trochilidae_timer_start("db");
usleep(200);
trochilidae_timer_stop("db");
var_dump($r);
echo 'trochilidae_timer_stop', PHP_EOL;

var_dump(trochilidae_timer_get_info());

echo 'OK', PHP_EOL;
