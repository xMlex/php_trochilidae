--TEST--
trochilidae: сообщение больше DEFAULT_CAPACITY корректно достраивается
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
ini_set('display_errors', '0');
ini_set('trochilidae.server_list', ''); // без collectors: сеть не нужна

function tr_bytes_send() {
    ob_start();
    phpinfo(INFO_MODULES);
    $s = ob_get_clean();
    if (!preg_match('/Bytes send\s*=>\s*(\d+)\s+AVG:/', $s, $m)) {
        die("FAIL cannot read \"Bytes send\" from phpinfo\n");
    }
    return (int) $m[1];
}

// Первый flush - маленькое сообщение, укладывается в начальный буфер.
trochilidae_reset();
trochilidae_set_tag('small', 'x');
trochilidae_flush();
$small = tr_bytes_send();

// Второй flush - сообщение на 200 КБ: буфер обязан вырасти через
// tr_array_ensure_capacity (и ужаться обратно на следующем clear).
$payload = str_repeat('y', 200000);
trochilidae_reset();
trochilidae_set_tag('big', $payload);
trochilidae_flush();
$big = tr_bytes_send();

$delta = $big - $small;
echo "small=$small big=$big delta=$delta\n";
echo ($delta > 190000 && $delta < 210000)
    ? "OK message grown past DEFAULT_CAPACITY\n"
    : "FAIL delta $delta, expected ~200000\n";

// После уменьшения буфера путь clear (shrink) тоже должен работать.
trochilidae_reset();
trochilidae_flush();
$smallAgain = tr_bytes_send();
echo ($smallAgain > $big)
    ? "OK subsequent flush still works\n"
    : "FAIL subsequent flush did not send\n";
?>
--EXPECTF--
small=%d big=%d delta=%d
OK message grown past DEFAULT_CAPACITY
OK subsequent flush still works
