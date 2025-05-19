#!/bin/bash

BUILD_DIR="build"

echo "Cleaning..."

# Neu erstellen
mkdir "$BUILD_DIR" || {
  echo "Failed to create build dir" >&2
}

cd "$BUILD_DIR" || {
  echo "Failed to enter build dir" >&2
  exit 1
}

echo "Starting cmake"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_COMPILER=/usr/bin/clang || {
  echo "Failed to generate build files" >&2
  exit 1
}

echo "Building..."
cmake --build . --parallel 14 || {
  echo "Failed to build with cmake" >&2
  exit 1
}

echo "Finished build"
