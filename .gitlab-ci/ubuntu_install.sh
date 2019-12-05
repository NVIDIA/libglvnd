#!/bin/bash

set -e
set -o xtrace

export DEBIAN_FRONTEND=noninteractive

dpkg --add-architecture i386

apt-get update
apt-get install -y --no-remove --no-install-recommends \
    autoconf \
    automake \
    gcc-multilib \
    libtool \
    libx11-dev \
    libx11-dev:i386 \
    libxext-dev \
    libxext-dev:i386 \
    ninja-build \
    pkg-config \
    pkg-config-i686-linux-gnu \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    x11proto-gl-dev \
    xvfb

# We need a version of meson later than the 0.45 in ubuntu 18.04
pip3 install meson