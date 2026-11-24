--TEST--
trochilidae tag and timer keys with embedded NUL keep all bytes on the wire
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
stream_set_timeout($s, 3);
$name = stream_socket_get_name($s, false);
$port = (int) substr(strrchr($name, ':'), 1);
ini_set('trochilidae.server_list', '127.0.0.1:' . $port);

function readWord(string $d, int &$p): int {
    $v = unpack('V', substr($d, $p, 4))[1];
    $p += 4;
    return $v;
}
function readShort(string $d, int &$p): int {
    $v = unpack('v', substr($d, $p, 2))[1];
    $p += 2;
    return $v;
}
function readStr(string $d, int &$p): string {
    $n = readWord($d, $p);
    $s = substr($d, $p, $n);
    $p += $n;
    return $s;
}
function readTv(string $d, int &$p): void {
    $p += 8;
}

trochilidae_set_tag("a\0b", "v");
trochilidae_timer_start("c\0d");
trochilidae_timer_stop("c\0d");
trochilidae_flush();

$packet = stream_socket_recvfrom($s, 65535);
fclose($s);
if ($packet === false) {
    echo "FAIL: no packet received\n";
    exit;
}

$p = 21; // chunk header
readTv($packet, $p);                       // request_start_time
$p += 2;                                   // mode, request_method
$p += 8;                                   // mem_peak_usage
readTv($packet, $p);                       // executionTime
readTv($packet, $p);                       // CPU user
readTv($packet, $p);                       // CPU system
$p += 8;                                   // response_http_size
$p += 4;                                   // response code
for ($i = 0; $i < 4; $i++) readStr($packet, $p);  // host, domain, uri, request id
$argvCount = readShort($packet, $p);
for ($i = 0; $i < $argvCount; $i++) readStr($packet, $p);

$tagKeys = [];
$tagCount = readShort($packet, $p);
for ($i = 0; $i < $tagCount; $i++) {
    $tagKeys[] = readStr($packet, $p);
    readStr($packet, $p);
}
$timerKeys = [];
$timerCount = readShort($packet, $p);
for ($i = 0; $i < $timerCount; $i++) {
    $timerKeys[] = readStr($packet, $p);
    $p += 4; // startCount
    $p += 8; // totalExecutionTime
}

echo "tags=", implode(',', array_map('bin2hex', $tagKeys)), "\n";
echo "timers=", implode(',', array_map('bin2hex', $timerKeys)), "\n";
?>
--EXPECTF--
tags=610062
timers=630064
