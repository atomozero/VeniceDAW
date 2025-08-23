#!/bin/bash

# Generate SHA256 checksum for VeniceDAW recipe
# This script should be run after creating a GitHub release

VERSION="$1"

if [ -z "$VERSION" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 1.0.0"
    exit 1
fi

URL="https://github.com/atomozero/VeniceDAW/archive/v${VERSION}.tar.gz"
TEMP_FILE="/tmp/VeniceDAW-${VERSION}.tar.gz"

echo "Downloading VeniceDAW v${VERSION} for checksum calculation..."
curl -L "$URL" -o "$TEMP_FILE"

if [ $? -eq 0 ]; then
    CHECKSUM=$(sha256sum "$TEMP_FILE" | cut -d' ' -f1)
    echo ""
    echo "✅ SHA256 checksum for VeniceDAW v${VERSION}:"
    echo "$CHECKSUM"
    echo ""
    echo "Update veniceDAW.recipe with:"
    echo "CHECKSUM_SHA256=\"$CHECKSUM\""
    echo ""
    
    # Clean up
    rm "$TEMP_FILE"
else
    echo "❌ Failed to download release archive"
    echo "Make sure the GitHub release v${VERSION} exists"
    exit 1
fi