#include "server/epoll_server.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "logger/logger.hpp"
#include "utils/nonblocking.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <vector>

namespace server {
namespace {

constexpr int kMaxEvents = 64;
constexpr int kEpollWaitMs = 500;
constexpr std::size_t kMaxBufferSize = 1024 * 1024;
constexpr std::size_t kReadChunk = 4096;

[[noreturn]] void die(const char* msg) {
    logger::instance().error(msg, ": ", std::strerror(errno));
    std::exit(1);
}

} // namespace

EpollServer::EpollServer(const config::ServerConfig& config, const RequestPipeline& pipeline)
    : config_(config), pipeline_(pipeline) {
    setup_listen_socket();

    epoll_fd_.reset(::epoll_create1(EPOLL_CLOEXEC));
    if (!epoll_fd_) {
        die("epoll_create1() failed");
    }

    epoll_add(listen_fd_.get(), EPOLLIN);
}

void EpollServer::setup_listen_socket() {
    listen_fd_.reset(::socket(AF_INET, SOCK_STREAM, 0));
    if (!listen_fd_) {
        die("socket() failed");
    }

    int yes = 1;
    if (::setsockopt(listen_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        die("setsockopt(SO_REUSEADDR) failed");
    }

    if (!utils::set_nonblocking(listen_fd_.get())) {
        die("set_nonblocking(listen) failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(config_.port);

    if (::bind(listen_fd_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("bind() failed");
    }

    if (::listen(listen_fd_.get(), 128) < 0) {
        die("listen() failed");
    }
}

void EpollServer::epoll_add(const int fd, const uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, fd, &ev) < 0) {
        die("epoll_ctl(ADD) failed");
    }
}

void EpollServer::epoll_mod(const int fd, const uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &ev) < 0) {
        die("epoll_ctl(MOD) failed");
    }
}

void EpollServer::epoll_del(const int fd) {
    ::epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, fd, nullptr);
}

void EpollServer::close_connection(const int fd) {
    epoll_del(fd);
    ::close(fd);
    connections_.erase(fd);
}

void EpollServer::accept_connections() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd =
            ::accept(listen_fd_.get(), reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            if (errno == EINTR) {
                continue;
            }
            logger::instance().error("accept() failed: ", std::strerror(errno));
            return;
        }

        if (!utils::set_nonblocking(client_fd)) {
            ::close(client_fd);
            continue;
        }

        char ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

        auto state = std::make_unique<ConnectionState>();
        state->conn.fd = client_fd;
        state->conn.peer_ip = ip;
        state->conn.peer_port = ntohs(client_addr.sin_port);

        touch_activity(state->conn);
        logger::instance().debug("Accepted ", state->conn.peer_ip, ":", state->conn.peer_port);

        epoll_add(client_fd, EPOLLIN);
        connections_.emplace(client_fd, std::move(state));
    }
}

void EpollServer::start_write(ConnectionState& state, const std::string& wire, const bool keep_alive) {
    state->conn.write_buffer = wire;
    state->conn.write_offset = 0;
    state->conn.keep_alive = keep_alive;
    state->state = ConnState::Writing;
    epoll_mod(state->conn.fd, EPOLLOUT);
}

void EpollServer::process_requests(ConnectionState& state) {
    while (state->state == ConnState::Reading) {
        const auto request_size = http::complete_request_size(state->conn.read_buffer);
        if (!request_size) {
            return;
        }

        const std::string raw = state->conn.read_buffer.substr(0, *request_size);
        const auto request = http::parse_http_request(raw);

        http::HttpResponse response;
        bool keep_alive = false;

        if (!request) {
            response = http::make_response(400, "Bad Request\n");
            keep_alive = false;
            logger::instance().warn("Malformed request from ", state->conn.peer_ip);
        } else {
            keep_alive = http::should_keep_alive(*request);
            logger::instance().info(request->method, " ", request->path, " ", request->version,
                                    " from ", state->conn.peer_ip);
            response = pipeline_.process(*request, state->conn.peer_ip);
            response.headers["Connection"] = keep_alive ? "keep-alive" : "close";
        }

        state->conn.read_buffer.erase(0, *request_size);
        start_write(state, http::serialize(response), keep_alive);
        return;
    }
}

void EpollServer::handle_readable(const int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    ConnectionState& state = *it->second;
    if (state.state != ConnState::Reading) {
        return;
    }

    char buffer[kReadChunk];
    while (true) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            if (state.conn.read_buffer.size() + static_cast<std::size_t>(n) > kMaxBufferSize) {
                logger::instance().warn("Request too large from ", state.conn.peer_ip);
                close_connection(fd);
                return;
            }
            state.conn.read_buffer.append(buffer, buffer + n);
            touch_activity(state.conn);
            process_requests(state);
            if (state.state == ConnState::Writing) {
                return;
            }
            continue;
        }

        if (n == 0) {
            close_connection(fd);
            return;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }

        logger::instance().error("recv() failed: ", std::strerror(errno));
        close_connection(fd);
        return;
    }
}

void EpollServer::handle_writable(const int fd) {
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    ConnectionState& state = *it->second;
    if (state.state != ConnState::Writing) {
        return;
    }

    while (state.conn.write_offset < state.conn.write_buffer.size()) {
        const ssize_t n = ::send(fd,
                                 state.conn.write_buffer.data() + state.conn.write_offset,
                                 state.conn.write_buffer.size() - state.conn.write_offset,
                                 0);
        if (n > 0) {
            state.conn.write_offset += static_cast<std::size_t>(n);
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }

        logger::instance().error("send() failed: ", std::strerror(errno));
        close_connection(fd);
        return;
    }

    touch_activity(state.conn);
    logger::instance().debug("Finished response to ", state.conn.peer_ip);

    if (!state.conn.keep_alive) {
        close_connection(fd);
        return;
    }

    state.conn.write_buffer.clear();
    state.conn.write_offset = 0;
    state.state = ConnState::Reading;
    epoll_mod(fd, EPOLLIN);

    if (!state.conn.read_buffer.empty()) {
        process_requests(state);
    }
}

void EpollServer::touch_activity(Connection& conn) {
    conn.last_activity = std::chrono::steady_clock::now();
}

void EpollServer::check_timeouts() {
    if (config_.request_timeout_seconds == 0) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto limit = std::chrono::seconds(static_cast<int>(config_.request_timeout_seconds));

    std::vector<int> timed_out;
    for (const auto& entry : connections_) {
        if (now - entry.second->conn.last_activity > limit) {
            timed_out.push_back(entry.first);
        }
    }

    for (const int fd : timed_out) {
        logger::instance().warn("Request timeout, closing fd ", fd);
        close_connection(fd);
    }
}

void EpollServer::run(std::atomic<bool>& running) {
    epoll_event events[kMaxEvents];

    logger::instance().info("Epoll server listening on 0.0.0.0:", config_.port);
    logger::instance().info("I/O model: epoll (non-blocking, keep-alive enabled)");
    logger::instance().info("Static root: ", config_.static_root);
    logger::instance().info("Rate limit: ", config_.rate_limit_rps, " rps, burst ", config_.rate_limit_burst);
    logger::instance().info("Cache: ", config_.cache_max_entries, " entries, TTL ", config_.cache_ttl_seconds,
                            "s");
    logger::instance().info("Request timeout: ", config_.request_timeout_seconds, "s");
    logger::instance().info("Press Ctrl+C to stop");

    while (running.load()) {
        const int ready = ::epoll_wait(epoll_fd_.get(), events, kMaxEvents, kEpollWaitMs);
        check_timeouts();
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            die("epoll_wait() failed");
        }

        for (int i = 0; i < ready; ++i) {
            const int fd = events[i].data.fd;
            const uint32_t ev = events[i].events;

            if (fd == listen_fd_.get()) {
                accept_connections();
                continue;
            }

            if (ev & (EPOLLERR | EPOLLHUP)) {
                close_connection(fd);
                continue;
            }

            if (ev & EPOLLIN) {
                handle_readable(fd);
            }

            if (ev & EPOLLOUT) {
                handle_writable(fd);
            }
        }
    }

    logger::instance().info("Closing ", connections_.size(), " active connection(s)...");

    std::vector<int> fds;
    fds.reserve(connections_.size());
    for (const auto& entry : connections_) {
        fds.push_back(entry.first);
    }
    for (const int fd : fds) {
        close_connection(fd);
    }

    logger::instance().info("Epoll server stopped.");
}

} // namespace server
