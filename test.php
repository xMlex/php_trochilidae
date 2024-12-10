#!/usr/bin/env php
<?php
error_reporting(E_ALL);
//print_r($_SERVER);
echo 'trochilidae.enabled: ', ini_get('trochilidae.enabled'), PHP_EOL;
ini_set('trochilidae.enabled', '1');
echo 'trochilidae.enabled: ', ini_get('trochilidae.enabled'), PHP_EOL;
// ini_set('trochilidae.server_list', 'localhost,192.168.1.2:999');
ini_set('trochilidae.server_list', 'localhost');
echo 'trochilidae.server_list: ', ini_get('trochilidae.server_list'), PHP_EOL;

function generateRandomString($length = 10) {
//     $characters = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
//     $charactersLength = strlen($characters);
//     $randomString = '';
//
//     for ($i = 0; $i < $length; $i++) {
//         $randomString .= $characters[random_int(0, $charactersLength - 1)];
//     }

//     return bin2hex(random_bytes($length));
    return bin2hex(random_bytes($length));
//     return $randomString;
}

trochilidae_set_tag("controller", "testController");
trochilidae_set_tag("action", "testAction");
trochilidae_set_tag("company", $company = "testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany testCompany ");
// trochilidae_set_tag("testMemory", bin2hex(random_bytes(1024*64)));
// trochilidae_set_tag("testMemory", generateRandomString(1024*1*1024));
// trochilidae_set_tag("testMemory", generateRandomString(64*1024/32));
trochilidae_set_tag("testMemory", str_repeat($company, 1024));
trochilidae_set_tag("testMemoryBytes", random_bytes(1024*1024));
trochilidae_set_tag("convertTest", 654);
trochilidae_set_tag("ruTest", "Бу, испугался? не бойся.");

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

$hostname = bin2hex(random_bytes(256*1024));
// echo "trochilidae_set_hostname('$hostname')", PHP_EOL;
trochilidae_set_hostname('test');
trochilidae_set_hostname($hostname);

echo 'OK: ', memory_get_peak_usage(true) / 1024 / 1024 , 'Mb', PHP_EOL;
