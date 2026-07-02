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
3.  **PHP tests:** `make test`
4.  **C unit tests (network):**
    `gcc -I. -Itests/stubs -DHAVE_CONFIG_H -g -O0 tests/unit_test.c trochilidae/tr_network.c trochilidae/utils.c -o tests/unit_test -lm && ./tests/unit_test`
5.  **C unit tests (internal — tr_array, tr_timer):**
    `gcc -I. -Itests/stubs -DHAVE_CONFIG_H -g -O0 tests/tr_internal_test.c -o tests/tr_internal_test -lm && ./tests/tr_internal_test`
    *Note:* These use `tests/stubs/php.h` to mock the Zend API outside of PHP context.

## Stubs
- `tests/stubs/php.h`: Minimal stub for `emalloc`/`erealloc`/`efree`/`estrdup` and `zend_resource` — used by `tr_internal_test.c` to compile without the real PHP headers.
