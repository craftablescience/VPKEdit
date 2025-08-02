#!/bin/bash

# VPKEdit macOS Build Script
# This script sets up the environment and builds VPKEdit for macOS

set -e  # Exit on any error

echo "VPKEdit macOS Build Script"
echo "=========================="

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Check for required tools
echo "Checking for required tools..."

# Check for Xcode Command Line Tools
if ! xcode-select -p &> /dev/null; then
    echo "Error: Xcode Command Line Tools are required."
    echo "Please install them with: xcode-select --install"
    exit 1
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is required for installing dependencies."
    echo "Please install Homebrew from: https://brew.sh"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Installing via Homebrew..."
    brew install cmake
fi

# Check for Qt6
echo "Checking for Qt6..."
QT_PATH=""
if command -v brew &> /dev/null; then
    QT_PATH=$(brew --prefix qt@6 2>/dev/null || echo "")
fi

if [[ -z "$QT_PATH" ]]; then
    echo "Qt6 not found. Installing via Homebrew..."
    brew install qt@6
    QT_PATH=$(brew --prefix qt@6)
fi

echo "Using Qt6 at: $QT_PATH"

# Check for pkg-config (needed for some dependencies)
if ! command -v pkg-config &> /dev/null; then
    echo "Installing pkg-config..."
    brew install pkg-config
fi

# Create build directory
BUILD_DIR="$PROJECT_ROOT/build"
echo "Creating build directory at: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring project with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -DQT_BASEDIR="$QT_PATH" \
    -DCPACK_GENERATOR=DragNDrop \
    -DVPKEDIT_BUILD_INSTALLER=ON

echo "Configuration complete!"

# Build the project
echo "Building VPKEdit..."
cmake --build . --config Release --parallel $(sysctl -n hw.ncpu)

echo "Build complete!"

# Check if GUI app was built successfully
if [[ -d "vpkedit.app" ]]; then
    echo "✅ GUI application built successfully: $BUILD_DIR/vpkedit.app"
else
    echo "❌ GUI application build failed"
fi

# Check if CLI was built successfully
if [[ -f "vpkeditcli" ]]; then
    echo "✅ CLI application built successfully: $BUILD_DIR/vpkeditcli"
else
    echo "❌ CLI application build failed"
fi

echo ""
echo "Build Summary:"
echo "=============="
echo "Build directory: $BUILD_DIR"
echo "Qt6 location: $QT_PATH"
echo ""
echo "To run the GUI: open '$BUILD_DIR/vpkedit.app'"
echo "To run the CLI: '$BUILD_DIR/vpkeditcli --help'"
echo ""
echo "To create a distributable DMG:"
echo "  cd '$BUILD_DIR' && cpack"
