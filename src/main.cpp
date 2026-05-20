#include "router/router.hpp"
#include "server/client_handler.hpp"
#include "threadpool/thread_pool.hpp"
#include "utils/sync_stream.hpp"
#include "utils/unique_fd.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

namespace {

std::atomic<bool> g_running{true};

void on_signal(int) {
    g_running.store(false);
}

[[noreturn]] void die(const std::string& msg) {
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(1);
}

std::size_t parse_thread_count(int argc, char** argv) {
    if (argc < 3) {
        return 4;
    }
    const int count = std::stoi(argv[2]);
    if (count <= 0) {
        return 1;
    }
    return static_cast<std::size_t>(count);
}

} // namespace

int main(int argc, char** argv) {
    const uint16_t port = (argc >= 2) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;
    const std::size_t num_threads = parse_thread_count(argc, argv);
    const router::Router router = router::make_default_router();

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    utils::UniqueFd listen_fd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!listen_fd) {
        die("socket() failed");
    }

    int yes = 1;
    if (::setsockopt(listen_fd.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        die("setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(listen_fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("bind() failed");
    }

    if (::listen(listen_fd.get(), 128) < 0) {
        die("listen() failed");
    }

    threadpool::ThreadPool pool(num_threads);

    utils::log("HTTP server listening on 0.0.0.0:", port);
    utils::log("Thread pool size: ", num_threads);
    utils::log("Routes: GET /, GET /health");
    utils::log("Press Ctrl+C to stop");

    while (g_running.load()) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        utils::UniqueFd client_fd(
            ::accept(listen_fd.get(), reinterpret_cast<sockaddr*>(&client_addr), &client_len));

        if (!client_fd) {
            if (errno == EINTR) {
                continue;
            }
            die("accept() failed");
        }

        char ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        const std::string client_ip(ip);
        const uint16_t client_port = ntohs(client_addr.sin_port);

        utils::log("Accepted connection from ", client_ip, ":", client_port);

        pool.submit([client_fd = std::move(client_fd), &router, client_ip, client_port]() mutable {
            server::handle_client(std::move(client_fd), router, client_ip, client_port);
        });
    }

    utils::log("Shutting down: waiting for thread pool to finish...");
    pool.shutdown();
    utils::log("Server stopped.");
    return 0;
}
