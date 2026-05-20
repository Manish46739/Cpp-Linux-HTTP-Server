#pragma once

#include <cstdint>
#include <string>

namespace server {

struct Connection {
    int fd = -1;
    std::string peer_ip;
    uint16_t peer_port = 0;

    std::string read_buffer;
    std::string write_buffer;
    std::size_t write_offset = 0;

    bool keep_alive = false;
};

} // namespace server
