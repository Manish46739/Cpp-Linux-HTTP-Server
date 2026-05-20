#!/usr/bin/env bash
# Basic stress test against a running server (parallel curl).
set -euo pipefail

BASE_URL="${1:-http://127.0.0.1:8080}"
REQUESTS="${2:-100}"
CONCURRENCY="${3:-20}"

echo "Stress test: ${REQUESTS} requests, concurrency ${CONCURRENCY}"
echo "Target: ${BASE_URL}/health"

TMP="$(mktemp)"
seq 1 "${REQUESTS}" | xargs -P "${CONCURRENCY}" -I{} \
  curl -s -o /dev/null -w "%{http_code}\n" "${BASE_URL}/health" >> "${TMP}"

ok=$(grep -c '^200$' "${TMP}" || true)
limited=$(grep -c '^429$' "${TMP}" || true)
bad=$(grep -cv -E '^(200|429)$' "${TMP}" || true)

rm -f "${TMP}"

echo "200 OK: ${ok}, 429 rate-limited: ${limited}, other failures: ${bad}"

if [[ "${bad}" -ne 0 ]]; then
  exit 1
fi

echo "Stress test passed."
