#pragma once

#include "server/request_pipeline.hpp"
#include "utils/unique_fd.hpp"

#include <cstdint>
#include <string>

namespace server {

void handle_client(utils::UniqueFd client_fd,
                   RequestPipeline& pipeline,
                   const std::string& client_ip,
                   uint16_t client_port);

} // namespace server
