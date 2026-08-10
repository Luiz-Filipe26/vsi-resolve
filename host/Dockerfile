# syntax=docker/dockerfile:1.4
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    pkg-config \
    libasound2-dev \
    libpulse-dev \
    libdbus-1-dev \
    libudev-dev \
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
    libwayland-dev \
    libxkbcommon-dev \
    wayland-protocols \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
