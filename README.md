# Multithreaded HTTP Server (C++17)

Linux development environment via Docker. Currently at **Phase 4** (static files, structured logging, configuration).

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

CLI arguments override environment defaults:

- `./build/server [port] [worker_threads]`

## Configuration (environment variables)

| Variable | Default | Description |
|----------|---------|-------------|
| `SERVER_PORT` | `8080` | Listen port |
| `SERVER_THREADS` | `4` | Thread pool size |
| `SERVER_STATIC_ROOT` | `static` | Directory for static files |
| `SERVER_LOG_LEVEL` | `INFO` | `DEBUG`, `INFO`, `WARN`, `ERROR` |

Example:

```bash
export SERVER_LOG_LEVEL=DEBUG
export SERVER_STATIC_ROOT=static
./build/server
```

## Test routes (from Windows)

```powershell
curl.exe -v http://127.0.0.1:8080/
curl.exe -v http://127.0.0.1:8080/health
curl.exe -v http://127.0.0.1:8080/style.css
curl.exe -v http://127.0.0.1:8080/not-found
```

Expected:

| Path | Status | Notes |
|------|--------|-------|
| `/` | 200 | Serves `static/index.html` |
| `/health` | 200 | `ok` |
| `/style.css` | 200 | CSS from static folder |
| `/unknown` | 404 | No matching file |

Open `http://127.0.0.1:8080/` in a browser to see the HTML page.

Press **Ctrl+C** in the server terminal for graceful shutdown.

## Project layout (Phase 4)

```
include/
  config/       Server configuration (env + CLI)
  logger/       Structured, thread-safe logging
  http/         Request parser + response serializer
  router/       Routes + static file fallback
  server/       Client handler + static file serving
  threadpool/   Task queue + worker threads
  utils/        RAII helpers
static/         Public files (index.html, style.css, ...)
```

## Troubleshooting Docker

If you see `dockerDesktopLinuxEngine: The system cannot find the file specified`, start Docker Desktop and ensure WSL2 + Linux containers are enabled.
