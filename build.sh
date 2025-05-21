#!/bin/bash

echo "Starting cmake"
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ || {
  echo "Failed to generate build files" >&2
  exit 1
}

echo "Building..."
cmake --build build -- -j12 || {
  echo "Failed to build with cmake" >&2
  exit 1
}

echo "Finished build"
