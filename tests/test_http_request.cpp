#include "http/http_request.hpp"

#include <gtest/gtest.h>

namespace {

std::string sample_get_request() {
    return "GET /health HTTP/1.1\r\n"
           "Host: localhost\r\n"
           "Connection: close\r\n"
           "\r\n";
}

} // namespace

TEST(HttpRequestParser, ParsesValidGetRequest) {
    const auto request = http::parse_http_request(sample_get_request());
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->method, "GET");
    EXPECT_EQ(request->path, "/health");
    EXPECT_EQ(request->version, "HTTP/1.1");
    EXPECT_EQ(request->headers.at("host"), "localhost");
    EXPECT_EQ(request->headers.at("connection"), "close");
}

TEST(HttpRequestParser, CompleteRequestSizeMatchesFullBuffer) {
    const std::string raw = sample_get_request();
    const auto size = http::complete_request_size(raw);
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(*size, raw.size());
}

TEST(HttpRequestParser, IncompleteRequestReturnsNullopt) {
    const std::string partial = "GET / HTTP/1.1\r\nHost: localhost\r\n";
    EXPECT_FALSE(http::complete_request_size(partial).has_value());
    EXPECT_FALSE(http::parse_http_request(partial).has_value());
}

TEST(HttpRequestParser, ShouldKeepAliveForHttp11) {
    const auto request = http::parse_http_request(sample_get_request());
    ASSERT_TRUE(request.has_value());
    EXPECT_FALSE(http::should_keep_alive(*request));
}

TEST(HttpRequestParser, ShouldKeepAliveWhenHeaderPresent) {
    const std::string raw =
        "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    const auto request = http::parse_http_request(raw);
    ASSERT_TRUE(request.has_value());
    EXPECT_TRUE(http::should_keep_alive(*request));
}

TEST(HttpRequestParser, RejectsPathTraversalInParse) {
    const std::string raw = "GET /../secret HTTP/1.1\r\nHost: localhost\r\n\r\n";
    const auto request = http::parse_http_request(raw);
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->path, "/../secret");
}
