# PHP Trochilidae

A PHP extension for collecting and sending application metrics (timers, tags, request data) to a remote collector via UDP.

## Requirements

- PHP 7.4+
- php-dev (for compilation)

## Install

```bash
phpize
./configure --enable-trochilidae
make
make test
make install
```

Enable the extension in `php.ini`:

```ini
extension=trochilidae.so
```

## Quick start

```php
<?php

// Set tags and hostname
trochilidae_set_tag('env', 'production');
trochilidae_set_hostname('app01');

// Profile a section of code
trochilidae_timer_start('db_query');
// ... do work ...
trochilidae_timer_stop('db_query');

// Get timer stats
$info = trochilidae_timer_get_info('db_query');
// $info['execution_time'] – last lap in seconds
// $info['total']          – total accumulated time
// $info['start_count']
// $info['stop_count']

// Send collected metrics
trochilidae_flush();
```

## PHP Functions

| Function | Description |
|---|---|
| `trochilidae_set_tag(string $key, string $value)` | Set a custom tag |
| `trochilidae_set_hostname(string $hostname)` | Override the default hostname |
| `trochilidae_timer_start(string $name)` | Start or resume a named timer |
| `trochilidae_timer_stop(string $name)` | Stop a named timer (no-op if not running) |
| `trochilidae_timer_get_info(string $name)` | Get timer statistics as array |
| `trochilidae_flush()` | Send collected metrics to the collector |
| `trochilidae_reset()` | Reset all collected metrics |

## TODO

- ~~REQUEST_TIME from php~~
- ~~request id~~
- ~~timers~~
