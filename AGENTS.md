# Project Guidelines: php_trochilidae

This project is a C-based PHP extension for metrics collection.

## Project Structure
- `trochilidae/`: Contains core functional sub-components.
    - `tr_array.c/h`: Buffer and array manipulation (binary serialization).
    - `tr_network.c/h`: TCP/UDP socket communication.
    - `tr_timer.c/h`: High-resolution timers and Zend resource management.
    - `tr_hooks.c`: Hooks for SAPI integration (intercepting output/execution).
    - `utils.c/h`: Helper functions and utility macros.
- `docs/protocol.md`: Wire format specification for the UDP protocol.
- `tests/`: Contains `.phpt` files for integration testing.

## Core Conventions
- **Naming:** 
    - Internal functions: `tr_` prefix.
    - PHP-exposed functions: `trochilidae_` prefix.
- **State Management:** Use `ZEND_DECLARE_MODULE_GLOBALS` and access globals via the `TR_G()` macro.
- **Resources:** Use Zend resource registration (`zend_register_list_destructors_ex`) for automated lifecycle management.
- **Performance:** Maintain low overhead; use custom binary serialization over JSON where possible.
- **Error Handling:** Use `php_error_docref`.

## Memory Allocator Convention
- **`trochilidae/tr_network.c`** uses the **system allocator** (`malloc`/`free`/`strdup`), NOT Zend allocator. This is a standalone networking utility that must also compile with stubs.
- **`trochilidae/tr_array.c`**, **`trochilidae/tr_timer.c`**, and **`trochilidae/tr_hooks.c`** use the **Zend allocator** (`emalloc`/`efree`/`estrdup`).
- **`trochilidae.c`** uses **both**: data crossing the boundary with `tr_network.c` (`client->host`, `pairs`) uses system allocator to match; PHP-internal data (`request_id`) uses Zend allocator.
- **Callers must match the callee's allocator**: if `parse_domain_port_pairs` allocates with `malloc`, the caller must free with `free` (not `efree`). Never mix allocators for the same pointer.
- **Test stubs** (`tests/stubs/php.h`) map `emalloc → malloc`, `efree → free`, `estrdup → strdup` to allow internal test builds outside of PHP context.

## Build & Test Workflow
1.  **Configuration:** `phpize && ./configure --enable-trochilidae`
2.  **Compilation:** `make`
3.  **PHP .phpt tests:**
    `php -d extension=modules/trochilidae.so run-tests.php -d extension=modules/trochilidae.so tests/`
    *Note:* The `-d extension=...` flag must be passed **both** to `run-tests.php` (so it propagates to child processes) and to the parent process for the skip checks.
4.  **C unit tests (network):**
    `gcc -I. -Itests/stubs -DHAVE_CONFIG_H -g -O0 tests/unit_test.c trochilidae/tr_network.c trochilidae/utils.c -o tests/unit_test -lm && ./tests/unit_test`
5.  **C unit tests (internal — tr_array, tr_timer):**
    `gcc -I. -Itests/stubs -DHAVE_CONFIG_H -g -O0 tests/tr_internal_test.c -o tests/tr_internal_test -lm && ./tests/tr_internal_test`
    *Note:* These use `tests/stubs/php.h` to mock the Zend API outside of PHP context.

### E2E Tests (UDP protocol validation)

E2E tests verify that the extension correctly sends binary UDP packets (chunked protocol) by directing traffic to a local UDP test server that parses and validates the wire format.

1.  **Start the UDP test server:**
    ```bash
    php -d extension=modules/trochilidae.so tests/udp_test_server.php --port=30002 --timeout=3 --verbose
    ```
    - Listens on `udp://0.0.0.0:30002` for chunked trochilidae packets.
    - Reassembles chunks, parses the binary protocol, and prints the decoded fields.
    - Exit code `0` — packet received and valid; `1` — timeout/error.
    - Supports `--expect=FILE.json` for deterministic field validation.

2.  **Run PHP scripts that send metrics via the extension:**
    ```bash
    php -d extension=modules/trochilidae.so -d "trochilidae.server_list=127.0.0.1:30002" \
      -r 'trochilidae_set_tag("e2e", "test"); trochilidae_flush();'
    ```

3.  **Full automated E2E cycle** (as used in `tests/install_test.sh`):
    ```bash
    UDP_PORT=30008
    UDP_LOG=$(mktemp)
    php -d extension=modules/trochilidae.so tests/udp_test_server.php \
      --port=$UDP_PORT --timeout=3 --verbose > "$UDP_LOG" 2>&1 &
    UDP_PID=$!
    # wait for "ready" in log, then run test PHP scripts
    php -d extension=modules/trochilidae.so -d "trochilidae.server_list=127.0.0.1:$UDP_PORT" \
      -r 'trochilidae_set_tag("t", "1"); trochilidae_flush();'
    sleep 0.5
    kill $UDP_PID 2>/dev/null; wait $UDP_PID 2>/dev/null || true
    grep -q "=== Parsed packet ===" "$UDP_LOG" && echo "E2E PASS" || echo "E2E FAIL"
    rm -f "$UDP_LOG"
    ```

4.  **Multi-chunk E2E test** (verifies chunking with small `chunk_size`):

    Use `ini_set()` — setting `chunk_size` before `server_list` is required (see known bug below):
    ```bash
    UDP_PORT=30009
    UDP_LOG=$(mktemp)
    php -d extension=modules/trochilidae.so tests/udp_test_server.php \
      --port=$UDP_PORT --timeout=3 --verbose > "$UDP_LOG" 2>&1 &
    UDP_PID=$!
    sleep 0.5
    php -d extension=modules/trochilidae.so \
      -r '
    ini_set("trochilidae.chunk_size", 120);
    ini_set("trochilidae.server_list", "127.0.0.1:'$UDP_PORT'");
    for ($i = 0; $i < 50; $i++) {
        trochilidae_set_tag("k$i", str_repeat("x", 25));
    }
    trochilidae_flush();
    '
    sleep 0.8
    kill $UDP_PID 2>/dev/null; wait $UDP_PID 2>/dev/null || true
    chunks=$(grep -c "Received chunk" "$UDP_LOG")
    echo "Chunks: $chunks"
    grep -q "All chunks received" "$UDP_LOG" && echo "MULTI-CHUNK PASS" || echo "FAIL"
    rm -f "$UDP_LOG"
    ```
    Expected: ~21 chunks (99 bytes payload each with `chunk_size=120`, since `CHUNK_HEADER_SIZE=21`).

**Key points:**
- Tests that run with a real `trochilidae.server_list` setting emit UDP packets — those packets go to the configured server.
- For isolated .phpt tests that don't need a server, set `trochilidae.server_list=localhost` (the packets will be sent but never arrive, which is harmless).
- For full protocol validation, use the UDP test server as shown above.
- **Known bug**: `trochilidae.chunk_size` and `trochilidae.server_list` cannot both be set via `-d` flags — the INI handler for `server_list` fires before `chunk_size` is applied, so collectors always get the default `chunk_size=65507`. Always use `ini_set()` for both when testing small chunk sizes, with `chunk_size` set **before** `server_list`.

## Stubs
- `tests/stubs/php.h`: Minimal stub for `emalloc`/`erealloc`/`efree`/`estrdup` and `zend_resource` — used by `tr_internal_test.c` to compile without the real PHP headers.
