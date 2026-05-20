# Multithreaded HTTP Server (C++17)

Linux development environment via Docker. Currently at **Phase 3** (thread pool + concurrent client handling).

## Prerequisites (Windows host)

- Docker Desktop installed and **running**
- Linux containers enabled

## Quick start

From `server/` in PowerShell:

```powershell
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm -p 8080:8080 cpp-http-server bash
```

Inside the container:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/server 8080 4
```

Arguments: `port` (default 8080), `worker_threads` (default 4).

## Test routes (from Windows)

```powershell
curl.exe -v http://127.0.0.1:8080/
curl.exe -v http://127.0.0.1:8080/health
curl.exe -v http://127.0.0.1:8080/unknown
```

Expected:

| Path | Status | Body |
|------|--------|------|
| `/` | 200 | `Hello from C++ HTTP server` |
| `/health` | 200 | `ok` |
| `/unknown` | 404 | `Not Found` |

The server handles multiple clients **concurrently** via a thread pool. Press **Ctrl+C** for graceful shutdown (finishes in-flight requests, then exits).

## Concurrency smoke test (inside container)

```bash
for i in $(seq 1 20); do curl -s http://127.0.0.1:8080/health & done; wait
```

You should see overlapping "Worker handling ..." log lines from different threads.

## Project layout (Phase 3)

```
include/
  http/         HTTP request parser + response serializer
  router/       Path routing
  server/       Per-client request handling
  threadpool/   Task queue + worker threads
  utils/        RAII helpers, thread-safe logging
src/
  main.cpp      Accept loop enqueues client handlers
  http/
  router/
  server/
  threadpool/
```

## Troubleshooting Docker

If you see `dockerDesktopLinuxEngine: The system cannot find the file specified`, start Docker Desktop and ensure WSL2 + Linux containers are enabled.
