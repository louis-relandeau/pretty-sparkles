#!/bin/bash

BUILD_DIR="build"
CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=-O3 -march=native -ffast-math"

# Only configure if build dir is missing or CMakeLists.txt is newer than the cache
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "CMakeLists.txt" -nt "$BUILD_DIR/CMakeCache.txt" ]; then
  echo "Configuring..."
  cmake -S . -B "$BUILD_DIR" -DCMAKE_CXX_FLAGS="-O3 -march=native -ffast-math"
fi

cmake --build "$BUILD_DIR" --config Debug