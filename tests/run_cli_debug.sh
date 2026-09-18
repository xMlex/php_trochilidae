#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SO_PATH="${SO_PATH:-$ROOT_DIR/modules/trochilidae.so}"
VALGRIND_LOG="${VALGRIND_LOG:-$ROOT_DIR/tests/valgrind-cli.log}"

if [ ! -f "$SO_PATH" ]; then
  echo "FAIL: extension not found at $SO_PATH"
  exit 1
fi

echo "Running CLI under valgrind..."
valgrind --tool=memcheck \
  --leak-check=full \
  --track-origins=yes \
  --num-callers=40 \
  --error-limit=no \
  --log-file="$VALGRIND_LOG" \
  php -d "extension=$SO_PATH" -r '
    trochilidae_set_tag("debug", "cli");
    trochilidae_timer_start("cli_debug");
    trochilidae_timer_stop("cli_debug");
    trochilidae_flush();
    echo "CLI debug probe done\n";
  '

echo "Valgrind CLI log: $VALGRIND_LOG"
rg "Invalid read|Invalid write|Use of uninitialised|trochilidae|ERROR SUMMARY" "$VALGRIND_LOG" || true
