--TEST--
server_list INI value is never mutated by parsing; repeated ini_set keeps all collectors
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
$list = 'host1:1111,host2:2222,host3:3333';

function collector_list(): string {
    ob_start();
    phpinfo(INFO_MODULES);
    $info = ob_get_clean();
    preg_match_all('/Collector:\s*\d+\s*=>\s*(\S+)/', $info, $m);
    return implode(',', $m[1]);
}

function ini_unchanged(string $expected): string {
    return ini_get('trochilidae.server_list') === $expected ? 'yes' : 'no';
}

ini_set('trochilidae.server_list', $list);
echo 'ini_get=', ini_unchanged($list), ' collectors=', collector_list(), "\n";

$bad = 0;
for ($i = 0; $i < 1000; $i++) {
    ini_set('trochilidae.server_list', $list);
    if (ini_unchanged($list) !== 'yes') {
        $bad++;
    }
    /* Re-parsing the value read back from ini_get() used to see only host1. */
    ini_set('trochilidae.server_list', ini_get('trochilidae.server_list'));
    if (ini_unchanged($list) !== 'yes' || collector_list() !== $list) {
        $bad++;
    }
}
echo "iterations=1000 bad=$bad\n";
/* Dummy hosts cannot receive UDP; skip the automatic RSHUTDOWN flush. */
ini_set('trochilidae.enabled', '0');
?>
--EXPECT--
ini_get=yes collectors=host1:1111,host2:2222,host3:3333
iterations=1000 bad=0
