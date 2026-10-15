#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PHP_FPM_BIN=${PHP_FPM_BIN:-$(command -v php-fpm || true)}
FCGI_BIN=${FCGI_BIN:-$(command -v cgi-fcgi || true)}
SO_PATH=${SO_PATH:-$ROOT_DIR/modules/trochilidae.so}
REQUESTS=${REQUESTS:-100}

if [ -z "$PHP_FPM_BIN" ] || [ -z "$FCGI_BIN" ]; then
    echo "SKIP: php-fpm or cgi-fcgi is not installed"
    exit 0
fi
if [ ! -f "$SO_PATH" ]; then
    echo "FAIL: extension not found at $SO_PATH"
    exit 1
fi

TMP_DIR=$(mktemp -d)
PORT=$((19000 + $$ % 10000))
FPM_PID_FILE=$TMP_DIR/php-fpm.pid
FPM_LOG=$TMP_DIR/php-fpm.log
FPM_CONFIG=$TMP_DIR/php-fpm.conf
PHP_INI=$TMP_DIR/php.ini

cleanup() {
    if [ -n "${FPM_MASTER_PID:-}" ]; then
        kill "$FPM_MASTER_PID" 2>/dev/null || true
        wait "$FPM_MASTER_PID" 2>/dev/null || true
    fi
    rm -r "$TMP_DIR"
}
trap cleanup EXIT HUP INT TERM

cat >"$PHP_INI" <<EOF2
extension=$SO_PATH
trochilidae.enabled=1
EOF2

cat >"$FPM_CONFIG" <<EOF2
[global]
pid = $FPM_PID_FILE
error_log = $FPM_LOG
daemonize = no

[www]
user = www-data
group = www-data
listen = 127.0.0.1:$PORT
pm = dynamic
pm.min_spare_servers=2
pm.max_spare_servers=4
pm.max_children = 2
pm.max_requests = 2
catch_workers_output = yes
clear_env = no
EOF2

PHP_INI_SCAN_DIR= "$PHP_FPM_BIN" -F -c "$PHP_INI" -y "$FPM_CONFIG" \
    >"$TMP_DIR/php-fpm.stdout" 2>&1 &
FPM_MASTER_PID=$!

ready=0
for _ in $(seq 1 50); do
    if grep -q 'ready to handle connections' "$FPM_LOG" 2>/dev/null; then
        ready=1
        break
    fi
    if ! kill -0 "$FPM_MASTER_PID" 2>/dev/null; then
        break
    fi
    sleep 0.1
done
if [ "$ready" -ne 1 ]; then
    cat "$TMP_DIR/php-fpm.stdout" "$FPM_LOG" 2>/dev/null || true
    echo "FAIL: php-fpm did not start"
    exit 1
fi

i=1
while [ "$i" -le "$REQUESTS" ]; do
    if ! env \
        SCRIPT_FILENAME="$ROOT_DIR/tests/fpm_lifecycle_request.php" \
        SCRIPT_NAME=/fpm_lifecycle_request.php \
        REQUEST_METHOD=GET \
        REQUEST_URI="/fpm_lifecycle_request.php?i=$i" \
        QUERY_STRING="i=$i" \
        SERVER_PROTOCOL=HTTP/1.1 \
        GATEWAY_INTERFACE=CGI/1.1 \
        SERVER_NAME=localhost \
        SERVER_PORT=80 \
        HTTP_HOST="host-$i.local" \
        "$FCGI_BIN" -bind -connect "127.0.0.1:$PORT" >"$TMP_DIR/response"; then
        echo "FAIL: FastCGI request $i failed"
        cat "$TMP_DIR/response" "$FPM_LOG" 2>/dev/null || true
        exit 1
    fi
    if ! grep -q 'FPM_LIFECYCLE_OK' "$TMP_DIR/response"; then
        echo "FAIL: request $i did not complete"
        cat "$TMP_DIR/response" "$FPM_LOG" 2>/dev/null || true
        exit 1
    fi
    i=$((i + 1))
done

if grep -Eiq 'zend_mm_heap corrupted|signal 6|signal 11|SIGABRT|SIGSEGV' "$FPM_LOG"; then
    cat "$FPM_LOG"
    echo "FAIL: PHP-FPM reported a crash"
    exit 1
fi

echo "PASS: $REQUESTS requests completed in one PHP-FPM worker"
