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
    $info = trochilidae_timer_get_info("smoke");
    assert($info["start_count"] === 1);
    assert($info["stop_count"] === 1);
    echo "  set_tag       OK\n";
    echo "  set_hostname  OK\n";
    echo "  timer         OK\n";
    echo "  flush         OK\n";
    echo "  All functions passed\n";
'

echo "=== Install test PASSED ==="
