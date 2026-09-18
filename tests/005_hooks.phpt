--TEST--
Check hook capture includes called methods only
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--INI--
trochilidae.hook_list=date,curl_exec,DateTimeImmutable->format,DateTimeImmutable::format
--FILE--
<?php
$port = 31000 + random_int(0, 2000);
$logFile = tempnam(sys_get_temp_dir(), 'tr_udp_hooks_');

$cmd = escapeshellarg(PHP_BINARY) . ' '
    . escapeshellarg(__DIR__ . '/udp_test_server.php')
    . ' --port=' . $port
    . ' --timeout=4 --verbose';

$descriptors = [
    0 => ['pipe', 'r'],
    1 => ['file', $logFile, 'a'],
    2 => ['file', $logFile, 'a'],
];

$proc = proc_open($cmd, $descriptors, $pipes, __DIR__);
if (!is_resource($proc)) {
    unlink($logFile);
    die("failed to start udp_test_server\n");
}

fclose($pipes[0]);

$ready = false;
for ($i = 0; $i < 40; $i++) {
    $log = (string)@file_get_contents($logFile);
    if (strpos($log, "ready\n") !== false) {
        $ready = true;
        break;
    }
    usleep(100000);
}

if (!$ready) {
    proc_terminate($proc);
    proc_close($proc);
    $log = (string)@file_get_contents($logFile);
    unlink($logFile);
    die("udp_test_server is not ready\n" . $log);
}

ini_set("trochilidae.server_list", "127.0.0.1:$port");

$date = new DateTimeImmutable();
$date->format("Y-m-d H:i:s");
date("c");
trochilidae_flush();

$parsed = false;
for ($i = 0; $i < 40; $i++) {
    $log = (string)@file_get_contents($logFile);
    if (strpos($log, "=== Parsed packet ===") !== false) {
        $parsed = true;
        break;
    }
    usleep(100000);
}

proc_terminate($proc);
proc_close($proc);

$log = (string)@file_get_contents($logFile);
unlink($logFile);

if (!$parsed) {
    die("packet was not parsed\n" . $log);
}

if (!preg_match('/^hooks \((\d+)\):\s+(.+)$/m', $log, $m)) {
    die("hooks line not found\n" . $log);
}

$hookCount = (int) $m[1];
$hooksLine = trim($m[2]);

if ($hookCount !== 2 || $hooksLine === '(none)') {
    die("unexpected hook count: $hookCount\n" . $log);
}

if (!preg_match('/"date"\s*:\s*\{"call_count"\s*:\s*([1-9]\d*)/', $hooksLine)) {
    die("date hook missing or zero calls\n" . $log);
}
if (!preg_match('/"DateTimeImmutable->format"\s*:\s*\{"call_count"\s*:\s*([1-9]\d*)/', $hooksLine)) {
    die("DateTimeImmutable->format hook missing or zero calls\n" . $log);
}
if (preg_match('/"curl_exec"/', $hooksLine) || preg_match('/"DateTimeImmutable::format"/', $hooksLine)) {
    die("unexpected non-called hook present\n" . $log);
}

echo "OK\n";
?>
--EXPECT--
OK
