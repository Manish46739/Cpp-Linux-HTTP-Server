# Multithreaded HTTP Server (C++17)

Linux HTTP server built in phases: TCP → HTTP → thread pool → static files → epoll → middleware → **tests & Docker**.

## Quick start (development)

```powershell
cd "C:\Users\kumar\Desktop\Cpp Project\server"
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm -p 8080:8080 cpp-http-server bash
```

Inside the container:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/server 8080
```

## Phase 7 — Unit tests

```bash
# One-shot script
chmod +x scripts/run_tests.sh
./scripts/run_tests.sh

# Or manually
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Tests cover: HTTP parser/serializer, router, rate limiter, response cache, thread pool, request pipeline.

Run tests in Docker CI profile (builds Release + runs ctest):

```bash
docker compose -f docker/docker-compose.yml --profile test build test
```

## Phase 7 — Stress & concurrency scripts

With the server running:

```bash
chmod +x scripts/stress_test.sh scripts/concurrency_test.sh
./scripts/stress_test.sh http://127.0.0.1:8080 200 25
```

Concurrency validation (starts server, 50 parallel clients, exits):

```bash
./scripts/concurrency_test.sh
```

## Production Docker image (optimized)

Multi-stage build — small runtime image with binary + static files only:

```bash
docker compose -f docker/docker-compose.yml --profile prod build server
docker compose -f docker/docker-compose.yml --profile prod up server
```

Or:

```bash
docker build --target runtime -t cpp-http-server:prod .
docker run --rm -p 8080:8080 cpp-http-server:prod
```

## Configuration

| Variable | Default | Description |
|----------|---------|-------------|
| `SERVER_PORT` | `8080` | Listen port |
| `SERVER_THREADS` | `4` | Thread pool size (threadpool mode) |
| `SERVER_STATIC_ROOT` | `static` | Static files directory |
| `SERVER_LOG_LEVEL` | `INFO` | `DEBUG`, `INFO`, `WARN`, `ERROR` |
| `SERVER_IO_MODEL` | `epoll` | `epoll` or `threadpool` |
| `SERVER_ASYNC_LOG` | `true` | Background logging thread |
| `SERVER_RATE_LIMIT_RPS` | `50` | Per-IP rate (`0` = off) |
| `SERVER_RATE_LIMIT_BURST` | `100` | Token bucket burst |
| `SERVER_CACHE_MAX_ENTRIES` | `128` | Response cache size |
| `SERVER_CACHE_TTL_SECONDS` | `60` | Cache TTL |
| `SERVER_REQUEST_TIMEOUT_SECONDS` | `30` | Idle timeout (epoll) |

## Test routes

```powershell
curl.exe http://127.0.0.1:8080/
curl.exe http://127.0.0.1:8080/health
curl.exe http://127.0.0.1:8080/style.css
```

## Project layout

```
include/          Headers (http, router, server, threadpool, logger, config)
src/              Implementation
tests/            Google Test unit tests
scripts/          run_tests.sh, stress_test.sh, concurrency_test.sh
static/           Web assets
docker/           docker-compose.yml
Dockerfile        dev / builder / runtime stages
```

## Build options

| CMake option | Default | Description |
|--------------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build `server_tests` and enable CTest |

Press **Ctrl+C** for graceful shutdown.
