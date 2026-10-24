#pragma once

#include "config/server_config.hpp"
#include "server/connection.hpp"
#include "server/request_pipeline.hpp"
#include "utils/unique_fd.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace server {

class EpollServer {
public:
    EpollServer(const config::ServerConfig& config, RequestPipeline& pipeline);

    void run(std::atomic<bool>& running);

private:
    enum class ConnState { Reading, Writing };

    struct ConnectionState {
        Connection conn;
        ConnState state = ConnState::Reading;
    };

    void setup_listen_socket();
    void accept_connections();
    void handle_readable(int fd);
    void handle_writable(int fd);
    void process_requests(ConnectionState& state);
    void start_write(ConnectionState& state, const std::string& wire, bool keep_alive);
    void close_connection(int fd);
    void touch_activity(Connection& conn);
    void check_timeouts();

    void epoll_add(int fd, uint32_t events);
    void epoll_mod(int fd, uint32_t events);
    void epoll_del(int fd);

    config::ServerConfig config_;
    RequestPipeline& pipeline_;

    utils::UniqueFd listen_fd_;
    utils::UniqueFd epoll_fd_;

    std::unordered_map<int, std::unique_ptr<ConnectionState>> connections_;
};

} // namespace server
