FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    gdb \
    ninja-build \
    git \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace