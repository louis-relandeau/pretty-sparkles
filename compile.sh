#!/bin/bash

cmake -S . -B build -DCMAKE_CXX_FLAGS="-O3 -march=native -ffast-math"
cmake --build build --config Debug