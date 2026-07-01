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
trochilidae.enabled=1
trochilidae.server_list=collector1.local:30002,collector2.local:30002
```

## Configuration

| INI directive | Type | Default | Description |
|---|---|---|---|
| `trochilidae.enabled` | bool | `1` | Enable or disable the extension |
| `trochilidae.server_list` | string | empty | Comma-separated `host:port` pairs of collector servers |

## How it works

Trochilidae hooks into the PHP request lifecycle:

1. **RINIT** — resets metrics, collects pre-request data (tags, server vars)
2. During the request, you can add tags and use timers via PHP functions
3. **RSHUTDOWN** — automatically serialises and sends all collected metrics to the configured collector(s) via UDP

For CLI scripts, metrics are also sent on shutdown; call `trochilidae_flush()` explicitly if you need to send data before the end of the script.

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

// Get stats for all timers
$info = trochilidae_timer_get_info();
// $info['timers'][0]['startCount']        – start count
// $info['timers'][0]['stopCount']         – stop count
// $info['timers'][0]['totalExecutionTime'] – total time in seconds
// $info['timers'][0]['lastExecutionTime']  – last lap in seconds

//[optional] Send collected metrics (send by default, after execute script/request)
trochilidae_flush();
```

## PHP Functions

| Function | Description |
|---|---|
| `trochilidae_set_tag(string $key, string $value)` | Set a custom tag |
| `trochilidae_set_hostname(string $hostname)` | Override the default hostname |
| `trochilidae_timer_start(string $name)` | Start or resume a named timer |
| `trochilidae_timer_stop(string $name)` | Stop a named timer (no-op if not running) |
| `trochilidae_timer_get_info()` | Get all timer statistics as nested array |
| `trochilidae_flush()` | Send collected metrics to the collector |
| `trochilidae_reset()` | Reset all collected metrics |

## Protocol TODO

Planned improvements to the UDP wire format (`docs/protocol.md`):

| # | Change | Why |
|---|---|---|
| 1 | **Magic + version** `uint32 + uint8` in chunk header | Distinguish from random UDP noise; enable backward-compatible evolution |
| 2 | **tv_sec → 64-bit `long`** | Avoid Y2038 overflow (time_t is 64-bit on modern systems) |
| 3 | **Payload length `uint32`** at start of payload | Self-validating parser; detect corruption/desync early |
| 4 | **CRC32C or Adler-32** at end of payload | Detect bit errors invisible to UDP checksum |
| 5 | **Monotonic sequence number** `uint32` in chunk header | Detect lost datagrams; order requests from a single agent |
| 6 | **Chunk reassembly timeout** on collector | Upper bound for incomplete multi-chunk requests (GC) |
| 7 | **Drop reserved padding** (8 zero bytes in chunk header) | Save bandwidth (8 bytes per chunk) |
| 8 | **Implement `compressed` flag** | Payload compression for large requests (planned but unused)

