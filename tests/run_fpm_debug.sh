#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${IMAGE_TAG:-trochilidae-fpm-debug:local}"
DOCKERFILE_PATH="$ROOT_DIR/tests/docker-fpm-debug.Dockerfile"

if [ "${1:-}" = "--inside-container" ]; then
  ulimit -c unlimited || true

  set +e
  KEEP_TMP_DIR=1 REQUESTS="${REQUESTS:-20}" FCGI_TIMEOUT_SEC="${FCGI_TIMEOUT_SEC:-5}" \
    /work/tests/fpm_repro_test.sh
  status=$?
  set -e

  if [ "$status" -eq 0 ]; then
    echo "Debug run passed (no crash detected in baseline run)."
    exit 0
  fi

  echo "Baseline run failed. Re-running under valgrind for detailed trace..."
  VALGRIND_FPM_LOG="/tmp/valgrind-fpm.log"
  VALGRIND_CLI_LOG="/tmp/valgrind-cli.log"

  set +e
  KEEP_TMP_DIR=1 REQUESTS=1 FCGI_TIMEOUT_SEC=15 \
    PHP_BIN_WRAPPER="valgrind --tool=memcheck --track-origins=yes --num-callers=40 --error-limit=no --log-file=$VALGRIND_CLI_LOG" \
    PHP_FPM_WRAPPER="valgrind --tool=memcheck --track-origins=yes --num-callers=40 --error-limit=no --log-file=$VALGRIND_FPM_LOG" \
    /work/tests/fpm_repro_test.sh
  valgrind_status=$?
  set -e

  echo "Valgrind logs:"
  echo "  CLI: $VALGRIND_CLI_LOG"
  echo "  FPM: $VALGRIND_FPM_LOG"
  if [ -f "$VALGRIND_CLI_LOG" ]; then
    rg "Invalid read|Invalid write|Use of uninitialised|free\\(|trochilidae|ERROR SUMMARY" "$VALGRIND_CLI_LOG" || true
  fi
  if [ -f "$VALGRIND_FPM_LOG" ]; then
    rg "Invalid read|Invalid write|Use of uninitialised|free\\(|trochilidae|ERROR SUMMARY" "$VALGRIND_FPM_LOG" || true
  fi

  echo "Attempting to print core backtrace if available..."
  for candidate in /tmp/core* /work/core*; do
    if [ -f "$candidate" ]; then
      echo "Core file found: $candidate"
      gdb -q -batch \
        -ex "set pagination off" \
        -ex "thread apply all bt full" \
        "$(command -v php-fpm)" "$candidate" || true
      break
    fi
  done

  if [ "$valgrind_status" -ne 0 ]; then
    exit "$valgrind_status"
  fi
  exit "$status"
fi

echo "Building debug Docker image: $IMAGE_TAG"
docker build -f "$DOCKERFILE_PATH" -t "$IMAGE_TAG" "$ROOT_DIR"

echo "Running valgrind/gdb FPM debug session in container"
docker run --rm \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --name "trochilidae-fpm-debug-run" \
  "$IMAGE_TAG"
