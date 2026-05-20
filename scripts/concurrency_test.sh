#!/usr/bin/env bash
# Starts the server, hammers it with parallel clients, then shuts down.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${PORT:-8080}"
BASE_URL="http://127.0.0.1:${PORT}"
SERVER_BIN="${SERVER_BIN:-${ROOT_DIR}/build/server}"

if [[ ! -x "${SERVER_BIN}" ]]; then
  echo "Server binary not found: ${SERVER_BIN}"
  echo "Build first: cmake --build build -j"
  exit 1
fi

cd "${ROOT_DIR}"

export SERVER_IO_MODEL=epoll
export SERVER_RATE_LIMIT_RPS=0
export SERVER_LOG_LEVEL=ERROR
export SERVER_ASYNC_LOG=false

echo "Starting server on port ${PORT}..."
"${SERVER_BIN}" "${PORT}" &
SERVER_PID=$!

cleanup() {
  if kill -0 "${SERVER_PID}" 2>/dev/null; then
    kill "${SERVER_PID}" 2>/dev/null || true
    wait "${SERVER_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT

sleep 0.5

echo "Running parallel health checks..."
ok=0
fail=0
for i in $(seq 1 50); do
  (
    code=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/health" || echo "000")
    echo "${code}"
  ) &
done > /tmp/concurrency_codes.txt
wait

while read -r code; do
  if [[ "${code}" == "200" ]]; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
  fi
done < /tmp/concurrency_codes.txt

rm -f /tmp/concurrency_codes.txt

echo "Concurrency test: ${ok} succeeded, ${fail} failed"
if ((fail > 0)); then
  exit 1
fi

echo "Concurrency test passed."
