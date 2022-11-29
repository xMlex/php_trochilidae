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

echo 'OK', PHP_EOL;
