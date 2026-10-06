--TEST--
trochilidae: буфер выделяется один раз при старте модуля, flush ничего не выделяет
--SKIPIF--
<?php
if (!extension_loaded('trochilidae')) print "skip trochilidae not loaded\n";
if (!function_exists('exec')) print "skip exec() disabled\n";
if (PHP_SAPI !== 'cli') print "skip cli only\n";
if (!file_exists(__DIR__ . '/bufalloc.c')) print "skip tests/bufalloc.c missing\n";
if (!file_exists(rtrim(ini_get('extension_dir'), '/') . '/trochilidae.so')) print "skip trochilidae.so not found\n";
?>
--FILE--
<?php
$so = sys_get_temp_dir() . '/trochilidae_bufalloc_' . getmypid() . '.so';
$src = __DIR__ . '/bufalloc.c';
exec('cc -shared -fPIC -O2 -o ' . escapeshellarg($so) . ' ' . escapeshellarg($src) . ' -ldl 2>&1',
     $buildOut, $buildRc);
if ($buildRc !== 0 || !file_exists($so)) {
    die("skip cannot build allocation counter:\n" . implode("\n", $buildOut) . "\n");
}

$ext = rtrim(ini_get('extension_dir'), '/') . '/trochilidae.so';

// Запускаем php дважды: базовая линия ( flush запрещён вообще - остаётся
// только то, что выделил сам модуль) и сценарий из прода (200 reset+flush
// подряд). Разница счётчиков - это ровно то, что выделили флеши.
// Если буфер инициализируется в PHP_MINIT, разница обязана быть нулём.
$baseTmp = tempnam(sys_get_temp_dir(), 'tr_base_');
$baseCase = $baseTmp . '.php';
@unlink($baseTmp);
// enabled=0 отключает и явные flush, и автоматический flush в RSHUTDOWN -
// иначе базовая линия сама бы выделяла буфер и маскировала регрессию.
file_put_contents($baseCase,
    "<?php\nini_set('trochilidae.enabled', '0');\necho 'started';\n");

$flushTmp = tempnam(sys_get_temp_dir(), 'tr_flush_');
$flushCase = $flushTmp . '.php';
@unlink($flushTmp);
file_put_contents($flushCase, <<<'PHP'
<?php
error_reporting(E_ALL);
ini_set('display_errors', '0');
ini_set('log_errors', '0');
ini_set('memory_limit', '128M');
ini_set('trochilidae.server_list', '');   // без collectors: сеть не нужна
for ($i = 0; $i < 200; $i++) {
    trochilidae_set_tag('iteration', (string) $i);
    trochilidae_reset();
    trochilidae_flush();
}
echo "flushed";
PHP);

function tr_counter_run($so, $ext, $case) {
    $cmd = 'LD_PRELOAD=' . escapeshellarg($so)
         . ' ' . escapeshellarg(PHP_BINARY)
         . ' -d extension=' . escapeshellarg($ext)
         . ' -d memory_limit=128M ' . escapeshellarg($case) . ' 2>/dev/null';
    exec($cmd, $out, $rc);
    return implode("\n", $out);
}

function tr_counter_parse($output) {
    if (!preg_match('/^BUFALLOC old_alloc=(\d+) old_free=(\d+) new_alloc=(\d+) new_free=(\d+) live=(\d+)$/m',
                    $output, $m)) {
        return null;
    }
    return [
        'old_alloc' => (int) $m[1],
        'new_alloc' => (int) $m[3],
        'live' => (int) $m[5],
    ];
}

$baselineOutput = tr_counter_run($so, $ext, $baseCase);
$flushOutput = tr_counter_run($so, $ext, $flushCase);

@unlink($so);
@unlink($baseCase);
@unlink($flushCase);

$base = tr_counter_parse($baselineOutput);
$case = tr_counter_parse($flushOutput);
if ($base === null || $case === null) {
    die("FAIL no counter report\n--- baseline ---\n$baselineOutput\n--- flush case ---\n$flushOutput\n");
}
if (strpos($flushOutput, 'flushed') === false) {
    die("FAIL the flush loop did not finish, output:\n$flushOutput\n");
}

echo "baseline_new_alloc={$base['new_alloc']}\n";
echo "case_new_alloc={$case['new_alloc']}\n";
echo "case_old_alloc={$case['old_alloc']}\n";
echo "case_live={$case['live']}\n";

// Главное: 200 flush'ей не добавили НИ ОДНОГО выделения буфера.
echo ($case['new_alloc'] === $base['new_alloc'])
    ? "OK flush adds zero allocations\n"
    : "FAIL flush allocated " . ($case['new_alloc'] - $base['new_alloc']) . " time(s)\n";
// Историческое число 5194910 больше никогда не запрашивается.
echo ($case['old_alloc'] === 0)
    ? "OK no 5194910 allocs\n"
    : "FAIL 5194910 allocated {$case['old_alloc']} time(s)\n";
// Буфер освобождён при выгрузке модуля - не переживает процесс.
echo ($case['live'] === 0)
    ? "OK buffer released at module unload\n"
    : "FAIL {$case['live']} buffer(s) still live\n";
?>
--EXPECTF--
baseline_new_alloc=%d
case_new_alloc=%d
case_old_alloc=0
case_live=0
OK flush adds zero allocations
OK no 5194910 allocs
OK buffer released at module unload
