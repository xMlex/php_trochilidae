#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PHP_BIN="${PHP_BIN:-php}"
PHP_FPM_BIN="${PHP_FPM_BIN:-}"
FCGI_BIN="${FCGI_BIN:-}"

if [ -z "$PHP_FPM_BIN" ]; then
  PHP_FPM_BIN="$(command -v php-fpm || true)"
fi

if [ -z "$FCGI_BIN" ]; then
  FCGI_BIN="$(command -v cgi-fcgi || true)"
fi

if [ -z "$PHP_FPM_BIN" ] || [ -z "$FCGI_BIN" ]; then
  echo "SKIP: php-fpm or cgi-fcgi is not installed"
  exit 0
fi

SO_PATH="${SO_PATH:-$ROOT_DIR/modules/trochilidae.so}"
REQUEST_SCRIPT="${REQUEST_SCRIPT:-$ROOT_DIR/tests/fpm_repro_request.php}"
CLI_REQUEST_SCRIPT="${CLI_REQUEST_SCRIPT:-$ROOT_DIR/tests/cli_repro_request.php}"

if [ ! -f "$SO_PATH" ]; then
  echo "FAIL: extension not found at $SO_PATH"
  exit 1
fi

TMP_DIR="$(mktemp -d)"
FPM_PORT="${FPM_PORT:-19000}"
UDP_PORT="${UDP_PORT:-19001}"
FPM_LOG="$TMP_DIR/fpm.log"
FPM_PID="$TMP_DIR/php-fpm.pid"
UDP_LOG="$TMP_DIR/udp.log"
CLI_LOG="$TMP_DIR/cli.log"
FPM_CONF="$TMP_DIR/php-fpm.conf"
POOL_CONF="$TMP_DIR/pool.conf"
FPM_SOCK="127.0.0.1:${FPM_PORT}"
REQUESTS="${REQUESTS:-60}"
CLI_REQUESTS="${CLI_REQUESTS:-60}"
FCGI_TIMEOUT_SEC="${FCGI_TIMEOUT_SEC:-5}"
CLI_TIMEOUT_SEC="${CLI_TIMEOUT_SEC:-5}"
TROCHILIDAE_ENABLED="${TROCHILIDAE_ENABLED:-1}"
KEEP_TMP_DIR="${KEEP_TMP_DIR:-0}"
PHP_NO_INI="${PHP_NO_INI:-0}"
PHP_BIN_WRAPPER="${PHP_BIN_WRAPPER:-}"
PHP_FPM_WRAPPER="${PHP_FPM_WRAPPER:-}"
FPM_HEALTH_CHECK_EVERY="${FPM_HEALTH_CHECK_EVERY:-10}"
POOL_USER="$(id -un)"
POOL_GROUP="$(id -gn)"
PHP_INI_FILE="$TMP_DIR/php.ini"
PHP_BIN_ARGS=()
PHP_FPM_ARGS=()
PHP_BIN_WRAPPER_ARGS=()
PHP_FPM_WRAPPER_ARGS=()

if [ "$PHP_NO_INI" = "1" ]; then
  PHP_BIN_ARGS+=("-n")
  PHP_FPM_ARGS+=("-n")
fi
if [ -n "$PHP_BIN_WRAPPER" ]; then
  read -r -a PHP_BIN_WRAPPER_ARGS <<< "$PHP_BIN_WRAPPER"
fi
if [ -n "$PHP_FPM_WRAPPER" ]; then
  read -r -a PHP_FPM_WRAPPER_ARGS <<< "$PHP_FPM_WRAPPER"
fi

if [ "$POOL_USER" = "root" ]; then
  POOL_USER="www-data"
fi
if [ "$POOL_GROUP" = "root" ]; then
  POOL_GROUP="www-data"
fi

CRASH_PATTERN="zend_mm_heap corrupted|exited on signal 6|exited on signal 11|SIGABRT|SIGSEGV|core dumped"

cleanup() {
  if [ -f "$FPM_PID" ]; then
    kill "$(cat "$FPM_PID")" 2>/dev/null || true
  fi
  if [ -n "${UDP_PID:-}" ]; then
    kill "$UDP_PID" 2>/dev/null || true
  fi
  wait "${UDP_PID:-0}" 2>/dev/null || true
  if [ "$KEEP_TMP_DIR" != "1" ]; then
    rm -rf "$TMP_DIR" || true
  else
    echo "TMP_DIR=$TMP_DIR"
  fi
}
trap cleanup EXIT

check_fpm_log_for_crash() {
  if rg -q "$CRASH_PATTERN" "$FPM_LOG"; then
    echo "FAIL: FPM crash signature found"
    rg "$CRASH_PATTERN" "$FPM_LOG" || true
    exit 1
  fi
}

run_cli_requests() {
  for i in $(seq 1 "$CLI_REQUESTS"); do
    if ! TROCHILIDAE_REQ_INDEX="$i" \
      timeout "$CLI_TIMEOUT_SEC" \
      "${PHP_BIN_WRAPPER_ARGS[@]}" "$PHP_BIN" "${PHP_BIN_ARGS[@]}" \
      -d "extension=$SO_PATH" \
      -d "trochilidae.enabled=$TROCHILIDAE_ENABLED" \
      -d "trochilidae.server_list=127.0.0.1:$UDP_PORT" \
      "$CLI_REQUEST_SCRIPT" "$i" >> "$CLI_LOG" 2>&1; then
      echo "FAIL: CLI request failed at iteration $i"
      rg "." "$CLI_LOG" || true
      exit 1
    fi
  done
}

cat > "$FPM_CONF" <<EOF
[global]
pid = $FPM_PID
error_log = $FPM_LOG
daemonize = yes
include = $POOL_CONF
EOF

cat > "$PHP_INI_FILE" <<EOF
extension=$SO_PATH
trochilidae.enabled=$TROCHILIDAE_ENABLED
trochilidae.server_list=127.0.0.1:$UDP_PORT
EOF

cat > "$POOL_CONF" <<EOF
[www]
user = $POOL_USER
group = $POOL_GROUP
listen = $FPM_SOCK
pm = dynamic
pm.max_children = 2
pm.start_servers = 1
pm.min_spare_servers = 1
pm.max_spare_servers = 2
pm.max_requests = 0
catch_workers_output = yes
clear_env = no
EOF

"${PHP_BIN_WRAPPER_ARGS[@]}" "$PHP_BIN" "${PHP_BIN_ARGS[@]}" -d "extension=$SO_PATH" "$ROOT_DIR/tests/udp_test_server.php" --port="$UDP_PORT" --timeout=8 --verbose > "$UDP_LOG" 2>&1 &
UDP_PID=$!

for _ in $(seq 1 40); do
  if rg -q "^ready$" "$UDP_LOG"; then
    break
  fi
  sleep 0.1
done

run_cli_requests

"${PHP_FPM_WRAPPER_ARGS[@]}" "$PHP_FPM_BIN" "${PHP_FPM_ARGS[@]}" -c "$PHP_INI_FILE" -y "$FPM_CONF"

for _ in $(seq 1 40); do
  if [ -f "$FPM_PID" ]; then
    break
  fi
  sleep 0.1
done

if [ ! -f "$FPM_PID" ]; then
  echo "FAIL: php-fpm did not start"
  rg "." "$FPM_LOG" || true
  exit 1
fi

for i in $(seq 1 "$REQUESTS"); do
  if ! QUERY_STRING="i=$i" \
    REQUEST_METHOD=GET \
    SCRIPT_FILENAME="$REQUEST_SCRIPT" \
    SCRIPT_NAME="/fpm_repro_request.php" \
    REQUEST_URI="/fpm_repro_request.php?i=$i" \
    SERVER_PROTOCOL=HTTP/1.1 \
    SERVER_NAME=localhost \
    HTTP_HOST=localhost \
    GATEWAY_INTERFACE=CGI/1.1 \
    timeout "$FCGI_TIMEOUT_SEC" "$FCGI_BIN" -bind -connect "$FPM_SOCK" > /dev/null; then
    echo "FAIL: cgi-fcgi request timed out or failed at iteration $i"
    rg "." "$FPM_LOG" || true
    exit 1
  fi
  if [ $((i % FPM_HEALTH_CHECK_EVERY)) -eq 0 ]; then
    check_fpm_log_for_crash
    if ! kill -0 "$(cat "$FPM_PID")" 2>/dev/null; then
      echo "FAIL: php-fpm master is not running at iteration $i"
      rg "." "$FPM_LOG" || true
      exit 1
    fi
  fi
done

sleep 0.5

check_fpm_log_for_crash

if ! rg -q "=== Parsed packet ===" "$UDP_LOG"; then
  echo "FAIL: no UDP packet was parsed"
  rg "." "$UDP_LOG" || true
  exit 1
fi

echo "FPM repro test passed"
