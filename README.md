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
    $data = json_decode($msg, true, 512, JSON_THROW_ON_ERROR);
    doWorkMethod($data);
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

TODO:
- ~~REQUEST_TIME from php~~
- request id
- ~~timers~~



