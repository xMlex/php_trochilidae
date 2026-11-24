--TEST--
trochilidae CLI daemon: reset opens a metric window per iteration
--SKIPIF--
<?php
if (!extension_loaded('trochilidae')) die('skip extension not loaded');
$s = @stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
if ($s === false) die("skip: cannot bind UDP socket: $errstr");
fclose($s);
?>
--STDIN--
{"n":0}
{"n":1}
{"n":2}
--FILE--
<?php
$s = stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
stream_set_timeout($s, 2);
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

// Typical long-running CLI worker: reset() acts as manual RINIT,
// flush() acts as manual RSHUTDOWN. One logical request per input line.
$counter = 0;
while (($msg = fgets(STDIN)) !== false) {
    trochilidae_reset();
    trochilidae_set_tag('calc_pipe', '1');
    trochilidae_set_tag('calc_iteration', $counter);

    $data = json_decode($msg, true, 512, JSON_THROW_ON_ERROR);
    usleep($data['n'] === 1 ? 120000 : 20000);

    trochilidae_flush();

    // Output written after flush() is not part of this metric window.
    echo 'OK', PHP_EOL;
    flush();
    $counter++;
}

$packets = [];
for ($i = 0; $i < $counter; $i++) {
    $packet = stream_socket_recvfrom($s, 65535);
    if ($packet === false) {
        break;
    }
    /* Chunk header: [id 8][chunk 2][total 2][compressed 1][key 8] = 21 bytes. */
    $p = 21;
    $start = readTv($packet, $p);
    $p += 2; // mode, request_method
    readWord($packet, $p); readWord($packet, $p); // mem_peak_usage
    $exec = readTv($packet, $p);
    readTv($packet, $p); readTv($packet, $p); // CPU user, system
    $resp = readWord($packet, $p); readWord($packet, $p);
    readWord($packet, $p); // response code
    readStr($packet, $p); readStr($packet, $p);
    readStr($packet, $p); readStr($packet, $p);
    $argc = readShort($packet, $p);
    for ($j = 0; $j < $argc; $j++) readStr($packet, $p);
    $tagc = readShort($packet, $p);
    $tags = [];
    for ($j = 0; $j < $tagc; $j++) {
        $key = readStr($packet, $p);
        $tags[$key] = readStr($packet, $p);
    }
    $timerc = readShort($packet, $p);
    $packets[] = [
        'start' => $start,
        'exec' => $exec,
        'resp' => $resp,
        'tags' => $tags,
    ];
}

// A flush without an intervening reset() must not produce another packet.
trochilidae_flush();
stream_set_blocking($s, false);
$extra = stream_socket_recvfrom($s, 65535);
stream_set_blocking($s, true);
fclose($s);

$starts = array_unique(array_column($packets, 'start'));
$tagsOk = count($packets) === 3;
foreach ($packets as $i => $packet) {
    $tagsOk = $tagsOk
        && ($packet['tags']['calc_pipe'] ?? null) === '1'
        && ($packet['tags']['calc_iteration'] ?? null) === (string) $i;
}

echo 'packets=', count($packets), "\n";
echo 'tags=', $tagsOk ? 'yes' : 'no', "\n";
echo 'per_iteration_exec=',
    ($packets[0]['exec'] >= 0.015 && $packets[0]['exec'] < 0.100
        && $packets[1]['exec'] >= 0.100 && $packets[1]['exec'] < 0.300
        && $packets[2]['exec'] >= 0.015 && $packets[2]['exec'] < 0.100)
    ? 'yes' : 'no', "\n";
echo 'response_after_flush_not_counted=',
    (count($packets) === 3 && $packets[0]['resp'] === 0
        && $packets[1]['resp'] === 0 && $packets[2]['resp'] === 0)
    ? 'yes' : 'no', "\n";
echo 'request_start_time_constant=', count($starts) === 1 ? 'yes' : 'no', "\n";
echo 'no_duplicate_flush=', $extra === false ? 'yes' : 'no', "\n";
?>
--EXPECT--
OK
OK
OK
packets=3
tags=yes
per_iteration_exec=yes
response_after_flush_not_counted=yes
request_start_time_constant=yes
no_duplicate_flush=yes
