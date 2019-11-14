#!/bin/bash

set -e
set -o xtrace

export DEBIAN_FRONTEND=noninteractive

dpkg --add-architecture i386

apt-get update
apt-get install -y --no-remove --no-install-recommends \
    autoconf \
    automake \
    pkg-config \
    libtool \
    libxext-dev \
    libx11-dev \
    x11proto-gl-dev \
    python3 \
    xvfb \
    gcc-multilib \
    libxext-dev:i386 \
    libx11-dev:i386

