--TEST--
trochilidae timers agree with total request execution time
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
function readTv(string $d, int &$p): float {
    $sec = readWord($d, $p);
    $usec = readWord($d, $p);
    return $sec + $usec / 1e6;
}

$wallStart = microtime(true);

trochilidae_timer_start('first');
usleep(100000);
trochilidae_timer_stop('first');

trochilidae_timer_start('second');
usleep(50000);
trochilidae_timer_stop('second');

$wallElapsed = microtime(true) - $wallStart;
$info = trochilidae_timer_get_info()['timers'];

trochilidae_flush();

$packet = stream_socket_recvfrom($s, 65535);
fclose($s);
if ($packet === false) {
    echo "FAIL: no packet received\n";
    exit;
}

/* Chunk header is 21 bytes: [id 8][chunk 2][total 2][compressed 1][key 8]. */
$p = 21;
readTv($packet, $p);          // request_start_time
$p += 2;                      // mode, request_method
readWord($packet, $p);        // mem_peak_usage (8 bytes)
readWord($packet, $p);
$exec = readTv($packet, $p);  // total request execution time
readTv($packet, $p);          // CPU user time
readTv($packet, $p);          // CPU system time
readWord($packet, $p);        // response_http_size (8 bytes)
readWord($packet, $p);
readWord($packet, $p);        // response code
readStr($packet, $p);         // hostname
readStr($packet, $p);         // domain
readStr($packet, $p);         // uri
readStr($packet, $p);         // request id
$argvCount = readShort($packet, $p);
for ($i = 0; $i < $argvCount; $i++) readStr($packet, $p);
$tagCount = readShort($packet, $p);
for ($i = 0; $i < $tagCount; $i++) { readStr($packet, $p); readStr($packet, $p); }
$timerCount = readShort($packet, $p);
$wire = [];
for ($i = 0; $i < $timerCount; $i++) {
    $timerName = readStr($packet, $p);
    $startCount = readWord($packet, $p);
    $wire[$timerName] = [$startCount, readTv($packet, $p)];
}

$t1 = $info[0]['totalExecutionTime'];
$t2 = $info[1]['totalExecutionTime'];
$sum = $t1 + $t2;

$checks = [
    'counts'    => $info[0]['startCount'] === 1 && $info[0]['stopCount'] === 1
                && $info[1]['startCount'] === 1 && $info[1]['stopCount'] === 1,
    'timer 1'   => $t1 >= 0.08 && $t1 <= 0.20,
    'timer 2'   => $t2 >= 0.035 && $t2 <= 0.15,
    'on wire'   => isset($wire['first'], $wire['second'])
                && $wire['first'][0] === 1 && $wire['second'][0] === 1
                && abs($wire['first'][1] - $t1) < 0.002
                && abs($wire['second'][1] - $t2) < 0.002,
    'sum <= exec' => $exec >= $sum - 0.005,
    'wall ~ exec' => $exec >= $wallElapsed - 0.005 && $exec <= $wallElapsed + 0.10,
];
$ok = true;
foreach ($checks as $label => $passed) {
    if (!$passed) {
        echo "FAILED: $label\n";
        $ok = false;
    }
}

printf("timers_ms=%d,%d elapsed_ms=%d execution_ms=%d sum_ms=%d wire_ms=%d,%d\n",
    (int) round($t1 * 1000), (int) round($t2 * 1000),
    (int) round($wallElapsed * 1000), (int) round($exec * 1000),
    (int) round($sum * 1000),
    (int) round($wire['first'][1] * 1000), (int) round($wire['second'][1] * 1000));
echo $ok ? "OK\n" : "FAIL\n";
?>
--EXPECTF--
timers_ms=%d,%d elapsed_ms=%d execution_ms=%d sum_ms=%d wire_ms=%d,%d
OK
