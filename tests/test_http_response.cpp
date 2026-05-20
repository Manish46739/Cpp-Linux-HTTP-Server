#include "http/http_response.hpp"

#include <gtest/gtest.h>

TEST(HttpResponse, SerializeIncludesStatusAndHeaders) {
    const http::HttpResponse response = http::make_response(200, "ok\n");
    const std::string wire = http::serialize(response);

    EXPECT_NE(wire.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(wire.find("Content-Length: 3"), std::string::npos);
    EXPECT_NE(wire.find("Connection: close"), std::string::npos);
    EXPECT_NE(wire.find("\r\n\r\nok\n"), std::string::npos);
}

TEST(HttpResponse, KeepAliveSetsHeader) {
    const http::HttpResponse response = http::make_response(200, "ok\n", "text/plain", true);
    const std::string wire = http::serialize(response);
    EXPECT_NE(wire.find("Connection: keep-alive"), std::string::npos);
}

TEST(HttpResponse, StatusTextFor429) {
    EXPECT_EQ(http::status_text(429), "Too Many Requests");
}
