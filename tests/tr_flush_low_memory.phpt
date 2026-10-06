--TEST--
trochilidae: flush в потоке, где скрипт уже съел ~120 МБ при memory_limit=128M
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
ini_set('memory_limit', '128M');

function tr_status_kb($key) {
    $s = @file_get_contents('/proc/self/status');
    if ($s === false) return -1;
    return preg_match('/^' . preg_quote($key, '/') . ":\s+(\d+) kB/m", $s, $m) ? (int) $m[1] : -1;
}
if (tr_status_kb('VmHWM') < 0) {
    die("FAIL /proc/self/status is unavailable on this system\n");
}

// Условие из прода: скрипт уже потребляет 120+ МБ, и только потом
// вызывается trochilidae_flush() 200 раз подряд (reset + flush).
$limit = 134217728; // 128M
$chunks = [];
while (($limit - memory_get_usage(true)) > 7 * 1048576) {
    $chunks[] = str_repeat('x', 1048576);
}
while (($limit - memory_get_usage(true)) > 5 * 1048576 + 131072) {
    $chunks[] = str_repeat('x', 65536);
}

$headroom = $limit - memory_get_usage(true);
echo "headroom=$headroom\n";
// Запас должен быть меньше старого буфера (5194910) и больше текущего
// DEFAULT_CAPACITY (9216), иначе тест ничего не доказывает.
if ($headroom <= 9216 || $headroom > 5194910) {
    die("FAIL headroom $headroom is outside (9216, 5194910]\n");
}

ini_set('trochilidae.server_list', ''); // без collectors: сеть не нужна

$hwmBefore = tr_status_kb('VmHWM');
for ($i = 0; $i < 200; $i++) {
    trochilidae_reset();
    trochilidae_flush();
}
$hwmAfter = tr_status_kb('VmHWM');
$delta = $hwmAfter - $hwmBefore;

echo "hwm_delta_kb=$delta\n";
echo ($delta >= 0 && $delta < 1024)
    ? "OK peak RSS stable\n"
    : "FAIL peak RSS grew by $delta kB\n";
?>
--EXPECTF--
headroom=%d
hwm_delta_kb=%d
OK peak RSS stable
