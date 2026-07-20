#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${IMAGE_TAG:-trochilidae-fpm-repro:local}"
DOCKERFILE_PATH="$ROOT_DIR/tests/docker-fpm-repro.Dockerfile"

echo "Building Docker image: $IMAGE_TAG"
docker build -f "$DOCKERFILE_PATH" -t "$IMAGE_TAG" "$ROOT_DIR"

echo "Running PHPT + FPM repro tests in container"
docker run --rm --name "trochilidae-fpm-repro-run" "$IMAGE_TAG"
