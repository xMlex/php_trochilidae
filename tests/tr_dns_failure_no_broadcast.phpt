--TEST--
failed collector DNS resolution must not attempt a broadcast destination
--SKIPIF--
<?php
if (!extension_loaded("trochilidae")) die("skip extension not loaded");
if (gethostbyname('no-such-host.invalid') !== 'no-such-host.invalid') {
    die("skip: resolver does not answer NXDOMAIN for .invalid");
}
?>
--FILE--
<?php
/* With the old retry-window behaviour the second window attempted
 * sendto(255.255.255.255) and printed a "sendto error" line before the
 * counter. The counter is captured via a handler; --EXPECTF-- requires that
 * no extra stderr line appears between the resolver message and the counter.
 * On Linux the broadcast attempt fails with EACCES, so a listening socket
 * cannot observe it — stderr is the only observable difference. */
$notices = 0;
set_error_handler(static function () use (&$notices): bool {
    $notices++;
    return true;
});
ini_set('trochilidae.server_list', 'no-such-host.invalid:12345');
for ($i = 0; $i < 2; $i++) {
    trochilidae_reset();
    trochilidae_flush();
}
restore_error_handler();
echo "notices=$notices\n";
?>
--EXPECTF--
tr_client_refresh_server: INADDR_NONE for %s
notices=2
