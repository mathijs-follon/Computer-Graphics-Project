#!/usr/bin/env bash

set -e

BUILD_DIR="../build"
BUILD_TYPE="Release"
TARGET="CG_OpenGL_Project"

echo "==> Using CMake:"
which cmake
cmake --version

echo "==> Creating build directory..."
mkdir -p $BUILD_DIR

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "==> Configuring project..."
    cmake -S . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi

echo "==> Building target: $TARGET"
cmake --build $BUILD_DIR --target $TARGET -- -j$(nproc)

echo "==> Done!"