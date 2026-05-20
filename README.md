# Multithreaded HTTP Server (C++17)

Linux development environment via Docker. Currently at **Phase 6** (async logging, rate limiting, caching, timeouts, middleware pipeline).

## Quick start

```powershell
cd "C:\Users\kumar\Desktop\Cpp Project\server"
docker compose -f docker/docker-compose.yml run --rm -p 8080:8080 cpp-http-server bash
```

Inside the container:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/server 8080 4
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
| `SERVER_RATE_LIMIT_RPS` | `50` | Requests/sec per IP (`0` = off) |
| `SERVER_RATE_LIMIT_BURST` | `100` | Token bucket burst size |
| `SERVER_CACHE_MAX_ENTRIES` | `128` | Response cache size (`0` = off) |
| `SERVER_CACHE_TTL_SECONDS` | `60` | Cache entry TTL |
| `SERVER_REQUEST_TIMEOUT_SECONDS` | `30` | Idle connection timeout (`0` = off) |

## Test

```powershell
curl.exe http://127.0.0.1:8080/
curl.exe http://127.0.0.1:8080/health
```

**Cache** — repeat the same GET; with `SERVER_LOG_LEVEL=DEBUG` you should see `Cache hit` in logs.

**Rate limit** — lower limits to test 429:

```bash
export SERVER_RATE_LIMIT_RPS=2
export SERVER_RATE_LIMIT_BURST=2
export SERVER_LOG_LEVEL=DEBUG
./build/server
# then run many rapid curl requests
```

## Request pipeline (middleware order)

```
1. Rate limiting (per client IP, token bucket)
2. Cache lookup (GET, 200 responses)
3. Router / static file handler
4. Cache store (on cacheable responses)
```

## Architecture

```
Client → epoll (non-blocking) → RequestPipeline → HTTP response
                ↓
         AsyncLogger (background thread)
```

Press **Ctrl+C** for graceful shutdown (drains async log queue).
