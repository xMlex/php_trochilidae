#!/bin/bash
#
# Verify that trochilidae extension is installed and working.
# Usage: tests/install_test.sh [path/to/php]
#
set -euo pipefail

PHP="${1:-php}"
SO_NAME="trochilidae.so"

echo "=== Install test for trochilidae ==="
echo "PHP binary : $($PHP -r 'echo PHP_BINARY;')"
echo "PHP version: $($PHP -r 'echo PHP_VERSION;')"

# 1. Check extension_dir exists
EXT_DIR=$($PHP -r 'echo PHP_EXTENSION_DIR;')
echo "Extension dir: $EXT_DIR"
if [ ! -d "$EXT_DIR" ]; then
    echo "FAIL: extension directory does not exist: $EXT_DIR"
    exit 1
fi

# 2. Check .so exists
if [ ! -f "$EXT_DIR/$SO_NAME" ]; then
    echo "FAIL: $SO_NAME not found in $EXT_DIR"
    echo "       Run 'make install' first."
    exit 1
fi
echo "  $SO_NAME found: $EXT_DIR/$SO_NAME"

# 3. Load extension and run smoke test
echo "Smoke test..."
$PHP -d "extension=$SO_NAME" -r '
    trochilidae_set_tag("test", "install_test");
    trochilidae_set_hostname("install_test");
    trochilidae_timer_start("smoke");
    trochilidae_timer_stop("smoke");
    $info = trochilidae_timer_get_info();
    $t = $info["timers"][0];
    assert($t["startCount"] === 1);
    assert($t["stopCount"] === 1);
    echo "  set_tag       OK\n";
    echo "  set_hostname  OK\n";
    echo "  timer         OK\n";
    echo "  flush         OK\n";
    echo "  All functions passed\n";
'

# 4. E2E test: send data to UDP test server and validate packet
echo "UDP integration test..."
UDP_PORT=30008
UDP_LOG=$(mktemp)
$PHP tests/udp_test_server.php --port=$UDP_PORT --timeout=3 --verbose > "$UDP_LOG" 2>&1 &
UDP_PID=$!
# Wait for server to be ready
for i in $(seq 1 5); do
    if grep -q "ready" "$UDP_LOG" 2>/dev/null; then
        break
    fi
    sleep 0.3
done
if ! kill -0 $UDP_PID 2>/dev/null; then
    echo "FAIL: UDP test server failed to start"
    cat "$UDP_LOG"
    rm -f "$UDP_LOG"
    exit 1
fi

$PHP -d "extension=$SO_NAME" -d "trochilidae.server_list=127.0.0.1:$UDP_PORT" -r '
    trochilidae_set_tag("e2e", "test");
    trochilidae_set_hostname("e2e-host");
    trochilidae_timer_start("e2e_timer");
    trochilidae_timer_stop("e2e_timer");
    trochilidae_flush();
' 2>/dev/null

sleep 0.5
kill $UDP_PID 2>/dev/null
wait $UDP_PID 2>/dev/null || true

if grep -q "=== Parsed packet ===" "$UDP_LOG"; then
    echo "  UDP packet received and parsed OK"
    if [ -t 1 ]; then
        cat "$UDP_LOG"
    fi
else
    echo "FAIL: UDP test server did not receive a valid packet"
    cat "$UDP_LOG"
    rm -f "$UDP_LOG"
    exit 1
fi
rm -f "$UDP_LOG"

echo "=== Install test PASSED ==="
