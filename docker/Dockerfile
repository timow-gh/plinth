# Build environment for plinth with GCC 14 and Clang 20
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# ── System packages ──────────────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Compilers
    build-essential \
    gcc-14 \
    g++-14 \
    # Build tools
    cmake \
    ninja-build \
    sccache \
    # Source / download tools
    git \
    curl \
    wget \
    zip \
    unzip \
    tar \
    pkg-config \
    # LLVM repo prerequisites
    ca-certificates \
    gnupg \
    lsb-release \
    # OpenGL development (for glad, imgui opengl3-binding)
    libgl-dev \
    mesa-common-dev \
    # X11 development (for glfw3)
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxext-dev \
    # Wayland development (for glfw3 Wayland backend)
    libwayland-dev \
    wayland-protocols \
    libxkbcommon-dev \
    # UUID library (for stduuid system-gen feature)
    uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# ── Clang 20 (from LLVM APT) ─────────────────────────────────────────────────
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor -o /usr/share/keyrings/llvm-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] \
        https://apt.llvm.org/noble/ llvm-toolchain-noble-20 main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        clang-20 \
        clang++-20 \
        clang-tidy-20 \
    && rm -rf /var/lib/apt/lists/*

# ── vcpkg ────────────────────────────────────────────────────────────────────
ENV VCPKG_ROOT=/opt/vcpkg

RUN git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

ENV PATH="$VCPKG_ROOT:$PATH"

# Binary cache for vcpkg: mount a volume here to persist compiled packages
# across container runs (see docker-compose.yml).
ENV VCPKG_DEFAULT_BINARY_CACHE=/cache/vcpkg-binary

# CPM source cache: mount a volume here to avoid re-cloning dependencies.
ENV CPM_SOURCE_CACHE=/cache/cpm

RUN mkdir -p /cache/vcpkg-binary /cache/cpm

WORKDIR /workspace
