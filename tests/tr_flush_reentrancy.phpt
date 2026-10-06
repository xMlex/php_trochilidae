--TEST--
trochilidae: вложенный flush из error-handler не роняет процесс (SIGSEGV)
--SKIPIF--
<?php if (!extension_loaded("trochilidae")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
ini_set('display_errors', '0');
ini_set('log_errors', '0');
// send() заведомо не проходит -> send_data() бросает E_NOTICE через
// php_error_docref. DNS не нужен: 255.255.255.255 даст INADDR_NONE сразу.
ini_set('trochilidae.server_list', '255.255.255.255');

$calls = 0;
set_error_handler(function () use (&$calls) {
    $calls++;
    if ($calls <= 10) {
        // Ровно то, что роняло процесс: повторный вход в send_data()
        // из обработчика notice, пока внешний ещё не освободил буфер.
        trochilidae_reset();
        trochilidae_flush();
    }
    return true;
});

trochilidae_flush();
echo "handler_calls=$calls\n";
echo "OK\n";
?>
--EXPECTF--
tr_client_refresh_server: INADDR_NONE for 255.255.255.255
tr_client_refresh_server: INADDR_NONE for 255.255.255.255
handler_calls=1
OK
