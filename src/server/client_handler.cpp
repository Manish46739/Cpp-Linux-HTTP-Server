#include "server/client_handler.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "logger/logger.hpp"

#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace server {

void handle_client(utils::UniqueFd client_fd,
                   const router::Router& router,
                   const std::string& client_ip,
                   const uint16_t client_port) {
    logger::instance().debug("Worker handling ", client_ip, ":", client_port);

    constexpr size_t kBufSize = 8192;
    char buf[kBufSize];

    const ssize_t n = ::recv(client_fd.get(), buf, sizeof(buf), 0);
    if (n < 0) {
        logger::instance().error("recv() failed for ", client_ip, ": ", std::strerror(errno));
        return;
    }
    if (n == 0) {
        logger::instance().warn("Client closed before sending data: ", client_ip);
        return;
    }

    const std::string raw(buf, buf + static_cast<size_t>(n));
    logger::instance().debug("Received ", n, " bytes from ", client_ip);

    const auto request = http::parse_http_request(raw);
    http::HttpResponse response;
    if (!request) {
        response = http::make_response(400, "Bad Request\n");
        logger::instance().warn("Malformed request from ", client_ip);
    } else {
        logger::instance().info(request->method, " ", request->path, " ", request->version,
                                " from ", client_ip);
        response = router.route(*request);
    }

    const std::string wire = http::serialize(response);
    const ssize_t sent = ::send(client_fd.get(), wire.data(), wire.size(), 0);
    if (sent < 0) {
        logger::instance().error("send() failed for ", client_ip, ": ", std::strerror(errno));
        return;
    }

    logger::instance().info("Sent HTTP/", response.status_code, " (", sent, " bytes) to ", client_ip);
}

} // namespace server
