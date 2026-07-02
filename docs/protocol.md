# Trochilidae UDP Protocol

## Transport Layer

Metrics are sent over **UDP** to one or more collector servers.

### Chunking

The payload is split into chunks if it exceeds `chunk_size` (configurable via `trochilidae.chunk_size` INI, default 65507). Each chunk is wrapped in a 21-byte header:

```
Offset  Size  Field         Description
──────────────────────────────────────────
 0       8    packetId      uint64 LE — random, same for all chunks of one request
 8       2    chunkIdx      uint16 LE — zero-based chunk index
10       2    totalChunks   uint16 LE — total number of chunks
12       1    compressed    uint8  — 0 = uncompressed, 1 = compressed (reserved)
13       2    magic         uint16 LE — "TR" (0x5452), identifies valid trochilidae packets
15       2    version       uint16 LE — protocol version (currently 1)
17       4    payloadLen    uint32 LE — total reassembled payload size (all chunks combined)
──────────────────────────────────────────
Total:   21 bytes
```

The full UDP datagram = `chunk_header(21) + chunk_payload(chunk_size - 21)`.

Receivers **must** validate `magic == 0x5452` and `version == 1` before processing a chunk.
Chunks with unknown magic or version are silently ignored.

## Serialization Primitives

All multi-byte integers are **little-endian** (native x86_64 byte order via `memcpy`).

| Name | Wire size | C type | Description |
|---|---|---|---|
| `byte` | 1 | `uint8_t` | Raw byte |
| `short` | 2 | `uint16_t` | Unsigned 16-bit LE |
| `word` | 4 | `uint32_t` | Unsigned 32-bit LE |
| `long` | 8 | `uint64_t` | Unsigned 64-bit LE |
| `tv` | 8 | `struct timeval` | `word(tv_sec)` + `word(tv_usec)` |
| `string` | 2 + N | `char[]` | `short(length)` + N raw bytes (no NUL terminator sent) |

## Payload Layout

The payload is a sequential byte buffer written in the fixed order below.

### Fixed Fields (54 bytes)

```
Offset  Size   Type    Field
─────────────────────────────────────────
  0      8      tv      request_start_time    — $_SERVER["REQUEST_TIME_FLOAT"]
  8      1      byte    modeType              — see § modeType
  9      1      byte    request_method        — see § request_method
 10      8      long    mem_peak_usage        — bytes (zend_memory_peak_usage)
 18      8      tv      executionTime         — wall-clock delta (seconds + microseconds)
 26      8      tv      CPUUsageUserTime      — ru_utime delta
 34      8      tv      CPUUsageSystemTime    — ru_stime delta
 42      8      long    response_http_size    — bytes written to response body
 50      4      word    responseCode          — HTTP response status code
─────────────────────────────────────────
Total:  54 bytes
```

### Strings Section

```
Offset   Type    Field          Description
─────────────────────────────────────────────
 54      string  hostName       gethostname()
 56+N    string  request_domain — $_SERVER["HTTP_HOST"] (CLI: PWD)
         string  request_uri    — $_SERVER["REQUEST_URI"] (CLI: SCRIPT_FILENAME)
         string  request_id     — X-Request-ID header (CLI: random 32-char hex)
```

### Variable-Length Sections

Each section begins with a `short` count, followed by that many records.

**argv** — command-line arguments:

```
short   count       number of arguments
string  argv[0]     first argument
string  argv[1]     ...
...
```

**tags** — user-defined key-value pairs (set via `trochilidae_set_tag`):

```
short   count       number of tags
string  key[0]      tag key
string  value[0]    tag value
string  key[1]      ...
...
```

**timers** — user-defined named timers (set via `trochilidae_timer_start/stop`):

```
short   count           number of timers
string  name[0]         timer name
word    startCount[0]   number of start() calls
tv      totalTime[0]    accumulated wall-clock time (totalExecutionTime)
string  name[1]         ...
...
```

**hooks** — internal function/method call interception counters:

```
short   count           number of successfully attached hooks
string  name[0]         hook name — function name or "ClassName->methodName"
long    callCount[0]    number of times the hook fired this request
tv      totalTime[0]    total wall-clock time spent in hooked calls
string  name[1]         ...
...
```

## Constants

### modeType

| Value | Meaning |
|---|---|
| `0x01` | CLI mode (PHP CLI binary) |
| `0x02` | CGI/FPM mode |

### request_method

| Value | Meaning |
|---|---|
| `0xFF` | UNDEFINED |
| `0x00` | NONE (CLI) |
| `0x01` | GET |
| `0x02` | HEAD |
| `0x03` | POST |
| `0x04` | PUT |
| `0x05` | DELETE |
| `0x06` | CONNECT |
| `0x07` | OPTIONS |
| `0x08` | TRACE |
| `0x09` | PATCH |

## Hook Name Format

- **Global functions**: `"curl_exec"` (exact function name)
- **Methods**: `"ClassName->methodName"` (always `->` separator, regardless of static/instance)

## Chunk Size Guidelines

| Setting | Environment |
|---|---|
| `1400` | Internet (avoids IP fragmentation, MTU-safe) |
| `9000` | Local network with Jumbo frames |
| `65507` | Loopback only (max UDP payload) |
