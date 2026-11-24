# PHP Trochilidae extension
A PHP extension for metrics collect

[![CodeFactor](https://www.codefactor.io/repository/github/xmlex/php_trochilidae/badge/main)](https://www.codefactor.io/repository/github/xmlex/php_trochilidae/overview/main)
[![Build Status](https://app.travis-ci.com/xMlex/php_trochilidae.svg?branch=main)](https://app.travis-ci.com/xMlex/php_trochilidae)

## Build and run

```
phpize && ./configure --enable-trochilidae && make && make test
php -d extension=`pwd`/modules/trochilidae.so -f test.php
run /home/mlex/CLionProjects/php_trochilidae/test.php
```

## Request lifecycle: CLI vs FPM

The extension collects one metric window per PHP request. `RINIT` opens the
window and `RSHUTDOWN` flushes it automatically, so under PHP-FPM (or any
short-lived SAPI) no explicit lifecycle calls are needed.

A long-running CLI process has only one PHP request for its whole lifetime, so
the application has to drive the lifecycle itself: call `trochilidae_reset()`
at the beginning of every logical request and `trochilidae_flush()` at the end.

| | PHP-FPM / web SAPI | Long-running CLI worker |
|---|---|---|
| Window opened by | `RINIT` (once per HTTP request) | `trochilidae_reset()` (once per loop iteration) |
| Window sent by | `RSHUTDOWN` | `trochilidae_flush()` |
| Need to call `reset()` | Normally no | Yes, before every iteration |
| `request_uri` / `request_domain` | `REQUEST_URI`, `HTTP_HOST` or `SERVER_NAME` | `SCRIPT_FILENAME`, `PWD` |
| Output size (`response_http_size`) | Bytes written during the request | Bytes written since the last `reset()` |

### `trochilidae_reset()` semantics

`reset()` opens a **new metric window**, not just clears tags and timers. It
re-snapshots the wall clock and CPU usage, resets the accumulated response
size, clears tags/timers and makes the window flushable again. That is
intentional: it is the manual equivalent of `RINIT` for CLI daemons.

Under FPM, `RINIT` has already opened the window. Calling `reset()` in the
middle of an HTTP request restarts it on purpose and discards timing, CPU and
output bytes collected so far — usually you do not want that there.

```php
<?php
// CLI daemon: one logical request per input line (manual RINIT/RSHUTDOWN).
while (($msg = fgets(STDIN)) !== false) {
    trochilidae_reset();                       // open a new window
    trochilidae_set_tag('calc_pipe', '1');
    doWorkMethod($msg);
    echo 'OK', PHP_EOL;                        // write output before flush
    trochilidae_flush();                       // send this window
}
```

### Gotchas for long-running processes

* `request_start_time` is read from `$_SERVER['REQUEST_TIME_FLOAT']`. In a CLI
  process that value is fixed at process start, so every window carries the
  same start time while `executionTime` is measured from the last `reset()`.
  Use a tag (for example an iteration number) to identify logical requests.
* Output written **after** `flush()` belongs to the next window and is
  discarded by the following `reset()`. Write output before flushing if it
  should be counted in `response_http_size`.
* A second `flush()` without an intervening `reset()` is a no-op: the extension
  sends at most one packet per window.

See `docs/decisions/001-reset-semantics.md` for why `reset()` keeps this
behaviour instead of being split into two functions.

## INI settings

| Name | Default | Description |
|---|---|---|
| `trochilidae.enabled` | `1` | Enable or disable collection. |
| `trochilidae.server_list` | — | Comma-separated `host:port` collector list. Parsing never modifies the INI value. |
| `trochilidae.dns_cache_ttl` | `600` | Seconds a resolved collector IP is reused before DNS is queried again. Values < 1 fall back to the default. |

## Wire format: `timeval` fields are 32-bit

`tr_array_write_tv()` serialises every `struct timeval` as two 4-byte words:
`tv_sec` is the **low 32 bits** of the platform's 64-bit `time_t`, and `tv_usec`
carries microseconds. This applies to `request_start_time`, `executionTime`,
`CPUUsageUserTime`, `CPUUsageSystemTime` and each timer's `totalExecutionTime`.

Most of these fields are **durations** (measured from the window or timer
start), so their values stay small and the 4-byte field is not a problem. The
exception is `request_start_time`, which is the absolute epoch value taken from
`$_SERVER['REQUEST_TIME_FLOAT']`. Collectors must read the seconds field as
**unsigned**: as a signed 32-bit timestamp it overflows on 2038-01-19, while the
raw 32 bits only wrap in 2106.

Widening the field to 8 bytes changes the packet layout and breaks existing
collectors. The format is deliberately left as-is; any change needs a
coordinated protocol version bump on both sides.

TODO:
- ~~REQUEST_TIME from php~~
- request id
- ~~timers~~



