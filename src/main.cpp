#include "config/server_config.hpp"
#include "logger/logger.hpp"
#include "router/router.hpp"
#include "server/client_handler.hpp"
#include "server/epoll_server.hpp"
#include "threadpool/thread_pool.hpp"
#include "utils/unique_fd.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
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

utils::UniqueFd create_listen_socket(const uint16_t port) {
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

    return listen_fd;
}

void run_thread_pool_server(const config::ServerConfig& cfg, const router::Router& router) {
    utils::UniqueFd listen_fd = create_listen_socket(cfg.port);
    threadpool::ThreadPool pool(cfg.thread_count);

    logger::instance().info("HTTP server listening on 0.0.0.0:", cfg.port);
    logger::instance().info("I/O model: thread pool (blocking sockets)");
    logger::instance().info("Thread pool size: ", cfg.thread_count);
    logger::instance().info("Static root: ", cfg.static_root);
    logger::instance().info("Press Ctrl+C to stop");

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

        logger::instance().debug("Accepted connection from ", client_ip, ":", client_port);

        pool.submit([client_fd = std::move(client_fd), &router, client_ip, client_port]() mutable {
            server::handle_client(std::move(client_fd), router, client_ip, client_port);
        });
    }

    logger::instance().info("Shutting down: waiting for thread pool to finish...");
    pool.shutdown();
}

} // namespace

int main(int argc, char** argv) {
    try {
        const config::ServerConfig cfg = config::ServerConfig::load(argc, argv);
        logger::init(cfg.log_level);
        const router::Router router = router::make_default_router(cfg);

        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        if (cfg.io_model == config::IoModel::Epoll) {
            server::EpollServer epoll_server(cfg, router);
            epoll_server.run(g_running);
        } else {
            run_thread_pool_server(cfg, router);
            logger::instance().info("Server stopped.");
        }
    } catch (const std::exception& ex) {
        std::cerr << "Configuration error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
