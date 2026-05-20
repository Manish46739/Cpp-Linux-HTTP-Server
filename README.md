# Multithreaded HTTP Server (C++17)

Linux development environment via Docker. Currently at **Phase 5** (epoll, non-blocking I/O, keep-alive).

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

Default I/O model is **epoll** (non-blocking, keep-alive). Thread pool mode is still available.

## Configuration (environment variables)

| Variable | Default | Description |
|----------|---------|-------------|
| `SERVER_PORT` | `8080` | Listen port |
| `SERVER_THREADS` | `4` | Thread pool size (threadpool mode only) |
| `SERVER_STATIC_ROOT` | `static` | Directory for static files |
| `SERVER_LOG_LEVEL` | `INFO` | `DEBUG`, `INFO`, `WARN`, `ERROR` |
| `SERVER_IO_MODEL` | `epoll` | `epoll` or `threadpool` |

Examples:

```bash
# Epoll mode (default)
./build/server 8080

# Thread pool mode (Phase 3 blocking model)
export SERVER_IO_MODEL=threadpool
./build/server 8080 4
```

## Test routes (from Windows)

```powershell
curl.exe -v http://127.0.0.1:8080/
curl.exe -v http://127.0.0.1:8080/health
curl.exe -v http://127.0.0.1:8080/style.css
```

### Test keep-alive (epoll mode)

```bash
# Inside container — multiple requests on one connection
printf 'GET /health HTTP/1.1\r\nHost: localhost\r\n\r\nGET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc 127.0.0.1 8080
```

You should see two HTTP responses before the connection closes.

Press **Ctrl+C** for graceful shutdown.

## Architecture (Phase 5)

```
┌─────────────────────────────────────┐
│  epoll_wait() event loop            │
│  - listen fd (accept, non-blocking) │
│  - client fds (EPOLLIN / EPOLLOUT)  │
└─────────────────────────────────────┘
         │ read buffer → parse HTTP
         │ router → serialize response
         │ write buffer → keep-alive or close
```

## Project layout

```
include/server/epoll_server.hpp   Linux epoll event loop
include/server/connection.hpp     Per-connection buffers
include/utils/nonblocking.hpp     fcntl O_NONBLOCK
include/threadpool/               Still used when SERVER_IO_MODEL=threadpool
```

## Troubleshooting Docker

If you see `dockerDesktopLinuxEngine: The system cannot find the file specified`, start Docker Desktop and ensure WSL2 + Linux containers are enabled.
