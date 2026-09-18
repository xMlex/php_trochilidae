<?php

declare(strict_types=1);

$requestIndex = (string)($argv[1] ?? getenv('TROCHILIDAE_REQ_INDEX') ?: '0');

$_SERVER['SCRIPT_FILENAME'] = '/tmp/cli_repro_' . $requestIndex . '.php';
$_SERVER['PWD'] = '/tmp/cli_repro_' . $requestIndex;

trochilidae_set_tag('scenario', 'cli_repro');
trochilidae_set_tag('request_seq', $requestIndex);
trochilidae_set_tag('transport', 'cli');

trochilidae_timer_start('cli_repro_timer');
usleep(1000);
trochilidae_timer_stop('cli_repro_timer');

trochilidae_flush();

echo "CLI_OK\n";
