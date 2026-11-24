<?php

declare(strict_types=1);

$_SERVER['HTTP_HOST'] = 'mutated-' . str_repeat('x', 40) . '-' . microtime(true);
$_SERVER['REQUEST_URI'] = '/mutated/' . bin2hex(random_bytes(16));
unset($_SERVER['HTTP_HOST']);
$_SERVER['HTTP_HOST'] = 'restored.local';

trochilidae_set_tag('case', 'fpm-lifecycle');
trochilidae_set_tag('oversized', str_repeat('x', 12000));
trochilidae_set_tag('123', 'numeric-key');
trochilidae_set_tag("bin\0key", 'binary-key');
trochilidae_timer_start('fpm_lifecycle_timer');
trochilidae_timer_stop('fpm_lifecycle_timer');
trochilidae_timer_start('7');
trochilidae_timer_stop('7');
trochilidae_timer_start("t\0imer");
trochilidae_timer_stop("t\0imer");

echo "FPM_LIFECYCLE_OK\n";
