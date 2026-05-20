cmake_minimum_required(VERSION 3.16)
project(cpp_http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(server
    src/main.cpp
    src/config/server_config.cpp
    src/http/http_request.cpp
    src/http/http_response.cpp
    src/logger/logger.cpp
    src/router/router.cpp
    src/server/client_handler.cpp
    src/server/static_file.cpp
    src/server/rate_limiter.cpp
    src/server/response_cache.cpp
    src/server/request_pipeline.cpp
    src/server/epoll_server.cpp
    src/utils/nonblocking.cpp
    src/threadpool/task_queue.cpp
    src/threadpool/thread_pool.cpp
)

target_include_directories(server PRIVATE include)

target_compile_options(server PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(server PRIVATE -O0 -g)
endif()
