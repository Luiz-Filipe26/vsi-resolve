# syntax=docker/dockerfile:1.4
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies, compression tools, and complete SDL3 dev headers
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    patchelf \
    ca-certificates \
    curl \
    tar \
    zip \
    pkg-config \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libxinerama-dev \
    libxss-dev \
    libxtst-dev \
    libxxf86vm-dev \
    libxfixes-dev \
    libdbus-1-dev \
    libudev-dev \
    libwayland-dev \
    libxkbcommon-dev \
    wayland-protocols \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libasound2-dev \
    libpulse-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
