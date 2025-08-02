#!/bin/bash

# Script to generate macOS .icns icon from PNG files
# Requires the sips utility (built into macOS)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
RES_DIR="$PROJECT_ROOT/res"
BRAND_DIR="$RES_DIR/brand"
ICONSET_DIR="$RES_DIR/logo.iconset"

# Create iconset directory
mkdir -p "$ICONSET_DIR"

# Check if we have source images
if [[ ! -f "$BRAND_DIR/logo_512.png" ]]; then
    echo "Error: logo_512.png not found in $BRAND_DIR"
    exit 1
fi

# Generate all required icon sizes
echo "Generating icon sizes..."

# Use logo_512.png as base
BASE_IMAGE="$BRAND_DIR/logo_512.png"

# Generate all required sizes for macOS icons
sips -z 16 16     "$BASE_IMAGE" --out "$ICONSET_DIR/icon_16x16.png"
sips -z 32 32     "$BASE_IMAGE" --out "$ICONSET_DIR/icon_16x16@2x.png"
sips -z 32 32     "$BASE_IMAGE" --out "$ICONSET_DIR/icon_32x32.png"
sips -z 64 64     "$BASE_IMAGE" --out "$ICONSET_DIR/icon_32x32@2x.png"
sips -z 128 128   "$BASE_IMAGE" --out "$ICONSET_DIR/icon_128x128.png"
sips -z 256 256   "$BASE_IMAGE" --out "$ICONSET_DIR/icon_128x128@2x.png"
sips -z 256 256   "$BASE_IMAGE" --out "$ICONSET_DIR/icon_256x256.png"
sips -z 512 512   "$BASE_IMAGE" --out "$ICONSET_DIR/icon_256x256@2x.png"
sips -z 512 512   "$BASE_IMAGE" --out "$ICONSET_DIR/icon_512x512.png"
cp "$BASE_IMAGE" "$ICONSET_DIR/icon_512x512@2x.png"

# Generate .icns file
echo "Generating .icns file..."
iconutil -c icns "$ICONSET_DIR" -o "$RES_DIR/logo.icns"

# Clean up iconset directory
rm -rf "$ICONSET_DIR"

echo "Generated $RES_DIR/logo.icns"
