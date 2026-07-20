--TEST--
Check tr_array growth and repeated flush/reset cycles
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--INI--
trochilidae.server_list=localhost
--FILE--
<?php
$ok = true;

for ($cycle = 0; $cycle < 8; $cycle++) {
    trochilidae_reset();
    for ($i = 0; $i < 80; $i++) {
        trochilidae_set_tag("k{$cycle}_{$i}", str_repeat("x", 48));
    }
    trochilidae_timer_start("memory");
    trochilidae_timer_stop("memory");
    $ok = $ok && trochilidae_flush();
}

echo $ok ? "array stress ok\n" : "array stress fail\n";
?>
--EXPECT--
array stress ok
