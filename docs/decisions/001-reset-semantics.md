# ADR-001: Keep `trochilidae_reset()` as a full metric-window reset

## Status

Accepted

## Date

2026-11-24

## Context

`trochilidae_reset()` calls `tr_reset()`, which clears tags/timers **and**
re-runs `collect_metrics_before_request()`: it re-snapshots wall clock and CPU
usage, zeroes `response_http_size`, re-reads request data from `$_SERVER`, and
clears the `flashed` guard so the window can be sent again.

This looked like a bug when called in the middle of a PHP-FPM request: a
`usleep(120000)` followed by `reset()` produced a packet with
`executionTime = 0.000010` instead of `~0.12`, and CPU time and output bytes
collected before the reset disappeared.

However, the extension is also used from long-running CLI workers that have a
single PHP request for their entire lifetime. There, the application drives the
lifecycle manually:

```php
while (($msg = fgets(STDIN)) !== false) {
    trochilidae_reset();       // manual RINIT for this logical request
    trochilidae_set_tag('calc_pipe', '1');
    doWorkMethod(json_decode($msg, true, flags: JSON_THROW_ON_ERROR));
    trochilidae_flush();       // manual RSHUTDOWN
}
```

Measured with three loop iterations (`usleep` of 100/150/200 ms):

```
packet #1  exec=0.100070
packet #2  exec=0.150071
packet #3  exec=0.200070
```

For this use case the full reset is exactly the desired behaviour: each
iteration gets its own timing, CPU and output window. The CLI and FPM scenarios
therefore attach different meaning to the same function — the caller decides
whether `reset()` is an iteration boundary or an accidental mid-request call.

## Decision

Keep the full-reset semantics of `trochilidae_reset()`. Document it as
"open a new metric window" and explain the CLI/FPM difference in the README.

Fix only the data errors that are wrong in both scenarios (for example the
`requestCount` being incremented both by `RINIT` and by `reset()`), without
removing the metric re-snapshot from `reset()`.

## Alternatives Considered

### Make `trochilidae_reset()` only clear tags and timers

- Pros: a stray mid-request call in FPM would no longer corrupt timing data.
- Cons: every CLI daemon must be rewritten, and its `executionTime` would grow
  monotonically from process start instead of being per-iteration.
- Rejected: this is a breaking change for the primary long-running use case.

### Add `trochilidae_begin_iteration()` and make `reset()` lightweight

- Pros: explicit intent for both scenarios, no ambiguity.
- Cons: new public API, one-line change required in every existing caller, and
  the real-world daemon code already uses `reset()` for this purpose.
- Rejected for now: revisit only if a second independent caller needs the
   lightweight variant.

## Consequences

* One function serves two intents; the caller must know which lifecycle model
  applies. This is documented in `README.md` ("Request lifecycle: CLI vs FPM").
* In a long-running CLI process `request_start_time` stays equal to the process
  start (from `$_SERVER['REQUEST_TIME_FLOAT']`) for every window, while
  `executionTime` is per-iteration. Logical requests must be identified by a
  tag such as an iteration number.
* Output produced after `flush()` is cleared by the next `reset()`; callers
  should write output before flushing if it must be counted.
* `requestCount` in `phpinfo()` currently overcounts by one per CLI iteration
  and by one per mid-request `reset()` in FPM. This is tracked separately and
  must be fixed without changing the window semantics described here.
