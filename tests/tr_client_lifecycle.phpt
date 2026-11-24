--TEST--
trochilidae failed client init leaves no dangling host or socket
--SKIPIF--
<?php
if (!extension_loaded("trochilidae")) die("skip extension not loaded");
if (!is_dir('/proc/self/fd')) die("skip: /proc/self/fd not available");
?>
--FILE--
<?php
$before = count(scandir('/proc/self/fd'));
ini_set('trochilidae.server_list', ':');
trochilidae_flush();
$after = count(scandir('/proc/self/fd'));
printf("new_fds=%d\n", $after - $before);
echo "OK\n";
?>
--EXPECTF--
tr_client_set_addr_info: not set client->host
new_fds=0
OK
