--TEST--
trochilidae request_start_time is encoded in microseconds
--SKIPIF--
<?php
if (!extension_loaded("trochilidae")) die("skip extension not loaded");
$s = @stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
if ($s === false) die("skip: cannot bind UDP socket: $errstr");
fclose($s);
?>
--FILE--
<?php
$s = stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
stream_set_timeout($s, 2);
$name = stream_socket_get_name($s, false);
$port = (int) substr(strrchr($name, ':'), 1);

ini_set('trochilidae.server_list', '127.0.0.1:' . $port);
trochilidae_flush();

$packet = stream_socket_recvfrom($s, 65535);
fclose($s);
if ($packet === false || strlen($packet) < 29) {
    echo "FAIL: no packet received\n";
    exit;
}

/* Chunk header: [id 8][chunk 2][total 2][compressed 1][key 8] = 21 bytes. */
$offset = 21;
$sec = unpack('V', substr($packet, $offset, 4))[1];
$usec = unpack('V', substr($packet, $offset + 4, 4))[1];

$time = $_SERVER['REQUEST_TIME_FLOAT'];
$expectedSec = (int) $time;
$expectedUsec = (int) (($time - $expectedSec) * 1e6);

printf("sec=%u usec=%u\n", $sec, $usec);
echo ($sec === $expectedSec && abs($usec - $expectedUsec) <= 2) ? "OK\n" : "FAIL\n";
?>
--EXPECTF--
sec=%d usec=%d
OK
