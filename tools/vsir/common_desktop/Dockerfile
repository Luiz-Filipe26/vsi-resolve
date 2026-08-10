# syntax=docker/dockerfile:1.4
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    patchelf \
    curl \
    tar \
    zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
