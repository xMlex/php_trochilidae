--TEST--
Check extension basic settings and initialization
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--INI--
trochilidae.enabled=0
--FILE--
<?php
echo 'trochilidae.enabled: ', ini_get('trochilidae.enabled'), PHP_EOL;
ini_set('trochilidae.enabled', '1');
echo 'trochilidae.enabled: ', ini_get('trochilidae.enabled'), PHP_EOL;
ini_set('trochilidae.server_list', 'localhost');
echo 'trochilidae.server_list: ', ini_get('trochilidae.server_list'), PHP_EOL;
ini_set('trochilidae.server_list', 'localhost,192.168.1.2:999');
echo 'trochilidae.server_list: ', ini_get('trochilidae.server_list'), PHP_EOL;
?>
--EXPECT--
trochilidae.enabled: 0
trochilidae.enabled: 1
trochilidae.server_list: localhost
trochilidae.server_list: localhost,192.168.1.2:999
