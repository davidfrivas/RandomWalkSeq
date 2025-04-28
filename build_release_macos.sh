#!/bin/bash

# RandomWalkSequencer macOS Build Script
# ======================================
# This script builds a release version of the RandomWalkSequencer plugin for macOS
# Builds only VST3 format
# Requires CMake 3.16 or higher

# Exit on any error
set -e

# Configuration
PLUGIN_NAME="RandomWalkSequencer"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
BUILD_DIR="cmake-build-release"

# Print header
echo "========================================"
echo "Building $PLUGIN_NAME for macOS (VST3)"
echo "========================================"

# Check CMake version
CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)

if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 16 ]); then
    echo "Error: CMake version 3.16 or higher is required."
    echo "Current version: $CMAKE_VERSION"
    exit 1
fi

echo "CMake version $CMAKE_VERSION detected - OK"

# Remove the plugin from your system if it exists
echo "Removing existing plugin installation..."
[ -d "$VST3_DIR/$PLUGIN_NAME.vst3" ] && rm -rf "$VST3_DIR/$PLUGIN_NAME.vst3"

# Clean your build directories
echo "Cleaning build directory..."
rm -rf "$BUILD_DIR"

# Recreate and rebuild in Release mode
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build VST3 version
echo "Building VST3 plugin..."
cmake --build . --target ${PLUGIN_NAME}_VST3 --config Release

echo "Copying plugin to system directory..."
# This path might need adjustment based on your CMake configuration
VST3_BUILD_PATH="./VST3/${PLUGIN_NAME}.vst3"

# Create directory if it doesn't exist
mkdir -p "$VST3_DIR"

# Copy built plugin to system directory
if [ -d "$VST3_BUILD_PATH" ]; then
    cp -r "$VST3_BUILD_PATH" "$VST3_DIR/"
    echo "VST3 plugin installed to $VST3_DIR"
else
    echo "Warning: VST3 plugin not found at $VST3_BUILD_PATH"
fi

# Validate the installation
echo "Validating plugin installation..."
[ -d "$VST3_DIR/$PLUGIN_NAME.vst3" ] && echo "✓ VST3 plugin installed successfully" || echo "✗ VST3 plugin installation failed"

echo "Build completed!"
echo "Plugin is ready to use in your DAW"