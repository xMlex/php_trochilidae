<?php

declare(strict_types=1);

trochilidae_set_tag('scenario', 'fpm_repro');
trochilidae_set_tag('request_seq', (string)($_GET['i'] ?? '0'));

// Intentionally mutate and replace server values captured at RINIT.
$_SERVER['HTTP_HOST'] = 'mutated-' . str_repeat('x', 40) . '-' . microtime(true);
$_SERVER['REQUEST_URI'] = '/mutated/' . ($_GET['i'] ?? '0') . '/' . bin2hex(random_bytes(4));

unset($_SERVER['HTTP_HOST']);
$_SERVER['HTTP_HOST'] = 'restored.local';

trochilidae_timer_start('fpm_repro_timer');
usleep(1000);
trochilidae_timer_stop('fpm_repro_timer');

echo "OK\n";
