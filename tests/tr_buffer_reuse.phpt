--TEST--
trochilidae serialization buffer grows and retains its high-water mark across oversized windows
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
stream_set_timeout($s, 5);
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

/* 12000 > DEFAULT_CAPACITY: forces buffer growth.
 * 300000 > MAX_RETAINED_CAPACITY: forces shrink-to-cap after the flush. */
foreach ([12000, 300000] as $len) {
    trochilidae_reset();
    trochilidae_set_tag('big', str_repeat('Z', $len));
    trochilidae_flush();

    $chunks = [];
    $total = null;
    while ($total === null || count($chunks) < $total) {
        $pkt = stream_socket_recvfrom($s, 65535);
        if ($pkt === false || strlen($pkt) < 21) {
            echo "len=$len FAIL: short or missing packet\n";
            break;
        }
        $idx = unpack('v', substr($pkt, 8, 2))[1];
        $total = unpack('v', substr($pkt, 10, 2))[1];
        $chunks[$idx] = substr($pkt, 21); // strip the 21-byte chunk header
    }
    if ($total === null || count($chunks) !== $total) {
        echo "len=$len FAIL: chunks missing\n";
        continue;
    }
    ksort($chunks);
    $msg = implode('', $chunks);

    $p = 8 + 2 + 8 + 8 + 8 + 8 + 8 + 4; // fixed fields from message start
    for ($i = 0; $i < 4; $i++) readStr($msg, $p); // host, domain, uri, request id
    $argvCount = readShort($msg, $p);
    for ($i = 0; $i < $argvCount; $i++) readStr($msg, $p);
    $tagCount = readShort($msg, $p);
    $got = '';
    for ($i = 0; $i < $tagCount; $i++) {
        $key = readStr($msg, $p);
        $value = readStr($msg, $p);
        if ($key === 'big') {
            $got = $value;
        }
    }
    printf("len=%d chunks=%d valid=%s\n", $len, $total,
        $got === str_repeat('Z', $len) ? 'yes' : 'NO');
}
fclose($s);
?>
--EXPECT--
len=12000 chunks=1 valid=yes
len=300000 chunks=5 valid=yes
