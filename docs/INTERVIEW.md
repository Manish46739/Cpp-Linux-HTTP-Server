# Interview talking points

Use this as a cheat sheet when discussing the project. Prefer **why** over reciting APIs.

## One-sentence pitch

A C++17 Linux HTTP server with an epoll event loop, optional thread pool, HTTP/1.1 keep-alive, middleware (rate limit + cache), async logging, Google Test coverage, and a multi-stage Docker image.

## Systems / networking

- **Listening vs connected sockets.** `accept()` returns a new fd; the listen fd stays in epoll for more clients.
- **Non-blocking I/O.** `recv`/`send`/`accept` return `EAGAIN`; that is not an error — wait for the next epoll event.
- **Level-triggered epoll** (this project). If data remains unread, `EPOLLIN` fires again. Edge-triggered (`EPOLLET`) requires draining until `EAGAIN` or you stall.
- **Keep-alive.** HTTP/1.1 defaults to persistent connections. Closing every request wastes handshakes; keeping the fd requires a read/write state machine and idle timeouts.
- **`SO_REUSEADDR`.** Lets you bind again quickly after restart while old sockets sit in `TIME_WAIT`.
- **HTTP framing.** Headers end at `\r\n\r\n`. Body length comes from `Content-Length`. Incomplete buffers must wait, not parse as 400.

## Concurrency

- **Thread-per-connection vs pool vs epoll.** Threads are simple until you have thousands of idle keep-alive sockets. Epoll scales connections without scaling threads 1:1.
- **Producer/consumer queue.** Accept thread (or epoll) produces work; workers consume. Shutdown uses `notify_all` so idle workers wake and exit.
- **Move-only tasks.** `std::function` requires copyable callables. Owning a socket fd in a lambda forced a custom `ITask` type.
- **Shared state.** `Router` is read-only after setup. Logger, cache, and rate limiter take mutexes. Prefer const refs over globals.

## Design / C++

- **RAII for FDs.** Every error path must close sockets; a destructor does that better than scattered `close()`.
- **Header/source split and namespaces** (`http`, `router`, `server`, `threadpool`, `logger`, `config`).
- **C++17:** structured bindings, `std::optional`, fold expressions in the logger, `std::filesystem` for static files.

## Performance

- Async logging: format on the caller, I/O on a background thread so the event loop is not blocked on `std::cout`.
- Response cache: LRU + TTL. Hits skip disk for static files. Invalidation is time-based only (no ETag/If-Modified-Since).
- Rate limiter: token bucket. Burst allows short spikes; refill rate caps sustained load. Cost is a mutex per `allow()` — fine until millions of distinct IPs (then you would shard or use a lock-free map).

## Testing

- Unit tests: parser, serializer, router, limiter, cache (including concurrent get/put), thread pool, pipeline 429.
- Stress: parallel `curl` against `/health`. Expect `200` or `429`, not crashes or hung fds.
- What I would add next: fuzz the parser, measure latency with `hey`/`wrk`, TLS via a reverse proxy.

## Honest limitations (interviewers like this)

- Single-threaded epoll loop: CPU-heavy handlers would block all connections. Next step: offload file I/O or compute to a pool while keeping epoll for sockets.
- No TLS, HTTP/2, or chunked encoding.
- Rate limiter map grows with unique IPs (no eviction of idle buckets yet).
- Windows host uses Docker because `epoll` is Linux-only.

Copy-paste resume lines: [RESUME.md](RESUME.md)
