#!/bin/bash

# Exit on any error
set -e

# Remove the plugin from your system
echo "Removing existing plugin..."
rm -rf ~/Library/Audio/Plug-Ins/VST3/RandomWalkSequencer.vst3

# Clean your build directories
echo "Cleaning build directories..."
rm -rf cmake-build-debug
rm -rf cmake-build-release

# Recreate and rebuild in Release mode
echo "Creating build directory..."
mkdir -p cmake-build-release
cd cmake-build-release

echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "Building plugin..."
cmake --build . --target RandomWalkSequencer_VST3 --config Release

echo "Build completed!"
