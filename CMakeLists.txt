cmake_minimum_required(VERSION 3.16)
project(cpp_http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SERVER_CORE_SOURCES
    src/config/server_config.cpp
    src/http/http_request.cpp
    src/http/http_response.cpp
    src/logger/logger.cpp
    src/router/router.cpp
    src/server/static_file.cpp
    src/server/rate_limiter.cpp
    src/server/response_cache.cpp
    src/server/request_pipeline.cpp
    src/utils/nonblocking.cpp
    src/threadpool/task_queue.cpp
    src/threadpool/thread_pool.cpp
)

add_library(server_core STATIC ${SERVER_CORE_SOURCES})
target_include_directories(server_core PUBLIC include)

target_compile_options(server_core PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
)

add_executable(server
    src/main.cpp
    src/server/client_handler.cpp
    src/server/epoll_server.cpp
)

target_link_libraries(server PRIVATE server_core)
target_compile_options(server PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(server PRIVATE -O0 -g)
    target_compile_options(server_core PRIVATE -O0 -g)
endif()

# --- Tests (Google Test) ---
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(server_tests
        tests/test_http_request.cpp
        tests/test_http_response.cpp
        tests/test_router.cpp
        tests/test_rate_limiter.cpp
        tests/test_response_cache.cpp
        tests/test_thread_pool.cpp
        tests/test_request_pipeline.cpp
    )

    find_package(Threads REQUIRED)
    target_link_libraries(server_tests PRIVATE server_core GTest::gtest_main Threads::Threads)
    target_compile_options(server_tests PRIVATE -Wall -Wextra -Wpedantic)

    include(GoogleTest)
    gtest_discover_tests(server_tests
        PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
