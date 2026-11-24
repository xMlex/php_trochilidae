--TEST--
trochilidae reset does not count as a new PHP request
--SKIPIF--
<?php if (!extension_loaded('trochilidae')) die('skip extension not loaded'); ?>
--FILE--
<?php
function requestCount(): int {
    ob_start();
    phpinfo(INFO_MODULES);
    $s = ob_get_clean();
    if (!preg_match('/Requests\s*=>\s*(\d+)/', $s, $m)) {
        return -1;
    }
    return (int) $m[1];
}

$before = requestCount();
trochilidae_reset();
$afterOne = requestCount();
trochilidae_reset();
trochilidae_reset();
$afterThree = requestCount();

printf("before=%d after_one=%d after_three=%d\n", $before, $afterOne, $afterThree);
echo ($before > 0 && $afterOne === $before && $afterThree === $before) ? "OK\n" : "FAIL\n";
?>
--EXPECTF--
before=%d after_one=%d after_three=%d
OK
