#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
PHP_IMAGE=${PHP_IMAGE:-php:8.5.11-fpm-alpine3.24}
REQUESTS=${REQUESTS:-500}

if ! command -v docker >/dev/null 2>&1; then
    echo "FAIL: docker CLI is not installed" >&2
    exit 1
fi
if ! docker info >/dev/null 2>&1; then
    echo "FAIL: cannot access the Docker daemon (check docker group/socket access)" >&2
    exit 1
fi

echo "Testing local source with image: $PHP_IMAGE"
docker run --rm --init \
    --env "REQUESTS=$REQUESTS" \
    --volume "$ROOT_DIR:/src:ro" \
    --workdir /src \
    --entrypoint /bin/sh \
    "$PHP_IMAGE" -euc '
        apk add --no-cache --virtual .extension-build-deps $PHPIZE_DEPS libtool

        mkdir -p /tmp/trochilidae-build/trochilidae /tmp/trochilidae-build/tests
        cp /src/config.m4 /src/php_trochilidae.h /src/trochilidae.c \
            /src/trochilidae_arginfo.h /tmp/trochilidae-build/
        cp /src/trochilidae/*.c /src/trochilidae/*.h \
            /tmp/trochilidae-build/trochilidae/
        cp -R /src/tests/. /tmp/trochilidae-build/tests/
        cp /src/run-tests.php /tmp/trochilidae-build/run-tests.php

        cd /tmp/trochilidae-build
        phpize
        ./configure --enable-trochilidae
        make
        NO_INTERACTION=1 make test TESTS=tests

        apk add --no-cache fcgi
        SO_PATH=/tmp/trochilidae-build/modules/trochilidae.so \
            REQUESTS="$REQUESTS" sh /src/tests/fpm_lifecycle.sh
    '
