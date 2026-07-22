ARG BASE_IMAGE=debian:bookworm
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        libdrm-dev \
        libegl1-mesa-dev \
        libgbm-dev \
        libgl1-mesa-dev \
        ninja-build \
        openjdk-17-jdk \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY tools/docker/arm64-jni-entrypoint.sh /usr/local/bin/arm64-jni-entrypoint
RUN chmod +x /usr/local/bin/arm64-jni-entrypoint

ENTRYPOINT ["/usr/local/bin/arm64-jni-entrypoint"]
