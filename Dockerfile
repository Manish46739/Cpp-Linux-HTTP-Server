FROM ubuntu:24.04 AS dev

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    gdb \
    ninja-build \
    git \
    ca-certificates \
    libgtest-dev \
    curl \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Default dev image: interactive shell with toolchain mounted at /workspace
CMD ["bash"]

FROM dev AS builder

COPY . /src
WORKDIR /src

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON \
 && cmake --build build -j \
 && ctest --test-dir build --output-on-failure

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/server ./server
COPY static ./static

EXPOSE 8080

ENV SERVER_PORT=8080
ENV SERVER_IO_MODEL=epoll

CMD ["./server", "8080"]
