--TEST--
DNS cache entry expires after trochilidae.dns_cache_ttl and re-resolves
--SKIPIF--
<?php
if (!extension_loaded("trochilidae")) die("skip extension not loaded");
if (!is_writable('/etc/hosts')) die("skip needs root to edit /etc/hosts");
$s = @stream_socket_server('udp://127.0.0.2:0', $errno, $errstr, STREAM_SERVER_BIND);
if ($s === false) die("skip cannot bind 127.0.0.2: $errstr");
fclose($s);
?>
--FILE--
<?php
$hostsFile = '/etc/hosts';
$alias = 'trdnscachetest';

/* Keep the original content and drop stale lines from a previous crashed run. */
$lines = explode("\n", file_get_contents($hostsFile));
$base = implode("\n", array_values(array_filter(
    $lines,
    static fn(string $line): bool => strpos($line, $alias) === false
)));
function set_host(string $ip, string $alias, string $base, string $hostsFile): void {
    file_put_contents($hostsFile, $base . $ip . ' ' . $alias . "\n");
}

$s1 = stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
$name = stream_socket_get_name($s1, false);
$port = (int) substr(strrchr($name, ':'), 1);
$s2 = stream_socket_server("udp://127.0.0.2:$port", $errno, $errstr, STREAM_SERVER_BIND);
stream_set_blocking($s1, false);
stream_set_blocking($s2, false);

function drain($s1, $s2): array {
    $count = [0, 0];
    $sockets = [$s1, $s2];
    $end = microtime(true) + 1.0;
    while (microtime(true) < $end) {
        $read = $sockets;
        $write = $except = null;
        if (@stream_select($read, $write, $except, 0, 100000) === 0) {
            continue;
        }
        foreach ($read as $socket) {
            $data = @stream_socket_recvfrom($socket, 65535);
            if ($data !== false && $data !== '') {
                $count[$socket === $s1 ? 0 : 1]++;
            }
        }
    }
    return $count;
}

try {
    set_host('127.0.0.1', $alias, $base, $hostsFile);
    ini_set('trochilidae.dns_cache_ttl', '1');
    ini_set('trochilidae.server_list', "$alias:$port");

    trochilidae_flush();
    $c = drain($s1, $s2);
    echo "first={$c[0]}:{$c[1]}\n";

    set_host('127.0.0.2', $alias, $base, $hostsFile);
    sleep(2);
    trochilidae_reset();
    trochilidae_flush();
    $c = drain($s1, $s2);
    echo "after_change={$c[0]}:{$c[1]}\n";

    for ($i = 0; $i < 50; $i++) {
        trochilidae_reset();
        trochilidae_flush();
    }
    $c = drain($s1, $s2);
    echo "cycle={$c[0]}:{$c[1]}\n";
} finally {
    file_put_contents($hostsFile, $base);
}

fclose($s1);
fclose($s2);
?>
--EXPECT--
first=1:0
after_change=0:1
cycle=0:50
