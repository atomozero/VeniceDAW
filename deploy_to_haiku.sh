#!/bin/bash
# deploy_to_haiku.sh - Deploy VeniceDAW to Haiku OS for native testing
# Usage: ./deploy_to_haiku.sh [haiku_user@haiku_host] [remote_path]

echo "🚀 VeniceDAW Haiku Deployment Script"
echo "===================================="

# Configuration
HAIKU_HOST="${1:-haiku@192.168.1.100}"  # Default Haiku host
REMOTE_PATH="${2:-/boot/home/VeniceDAW}"  # Default remote path

echo "Target Host: $HAIKU_HOST"
echo "Remote Path: $REMOTE_PATH"
echo

# Files to transfer (only essential for Haiku)
FILES_TO_TRANSFER=(
    # Core source files
    "src/audio/SimpleHaikuEngine.h"
    "src/audio/SimpleHaikuEngine.cpp"
    "src/audio/AdvancedAudioProcessor.h"
    "src/audio/AdvancedAudioProcessor.cpp"
    "src/audio/DSPAlgorithms.h"
    "src/audio/DSPAlgorithms.cpp"
    
    # GUI files
    "src/gui/Mixer3DWindow.h"
    "src/gui/Mixer3DWindow.cpp"
    "src/gui/SpatialMixer3DWindow.h"
    "src/gui/SpatialMixer3DWindow.cpp"
    "src/gui/SpatialControlPanels.cpp"
    
    # Test files
    "src/testing/SpatialAudioTest.cpp"
    "test_phase4_2_hrtf.cpp"
    "test_audio_playback.cpp"
    "test_3d_mixer.cpp"
    
    # Build system
    "Makefile.haiku"
    
    # Documentation
    "README.md"
    "CLAUDE.md"
)

echo "📦 Preparing files for transfer..."

# Create a temporary directory for deployment
TEMP_DIR="/tmp/veniceDAW_deploy_$$"
mkdir -p "$TEMP_DIR"

echo "🗂️  Copying files to temporary directory..."
for file in "${FILES_TO_TRANSFER[@]}"; do
    if [ -f "$file" ]; then
        # Create directory structure
        target_dir="$TEMP_DIR/$(dirname "$file")"
        mkdir -p "$target_dir"
        
        # Copy file
        cp "$file" "$TEMP_DIR/$file"
        echo "   ✓ $file"
    else
        echo "   ⚠️  Warning: $file not found"
    fi
done

# Create remote testing script
cat > "$TEMP_DIR/test_on_haiku.sh" << 'EOF'
#!/bin/bash
# test_on_haiku.sh - Run VeniceDAW tests on native Haiku OS

echo "🧪 VeniceDAW Native Haiku Test Suite"
echo "===================================="
echo

# Check if we're really on Haiku
if [[ "$(uname)" != "Haiku" ]]; then
    echo "❌ ERROR: This script must run on native Haiku OS!"
    echo "   Current system: $(uname -a)"
    exit 1
fi

echo "✅ Confirmed running on Haiku OS"
echo "   System: $(uname -a)"
echo

# Build and test
echo "🔧 Building VeniceDAW components..."
make -f Makefile.haiku clean
echo

echo "🎯 Running test suite..."
make -f Makefile.haiku test-all

echo
echo "🎉 VeniceDAW Haiku testing complete!"
EOF

chmod +x "$TEMP_DIR/test_on_haiku.sh"

echo
echo "📡 Transferring to Haiku host..."
echo "   Creating remote directory..."

# Create remote directory
ssh "$HAIKU_HOST" "mkdir -p $REMOTE_PATH" || {
    echo "❌ Failed to create remote directory"
    rm -rf "$TEMP_DIR"
    exit 1
}

# Transfer files
echo "   Uploading files..."
scp -r "$TEMP_DIR"/* "$HAIKU_HOST:$REMOTE_PATH/" || {
    echo "❌ Failed to transfer files"
    rm -rf "$TEMP_DIR"
    exit 1
}

echo "✅ Transfer complete!"
echo

echo "🧪 To run tests on Haiku:"
echo "   ssh $HAIKU_HOST"
echo "   cd $REMOTE_PATH"
echo "   ./test_on_haiku.sh"
echo

echo "🔧 To build manually on Haiku:"
echo "   make -f Makefile.haiku test-spatial"
echo "   make -f Makefile.haiku test-hrtf"
echo "   make -f Makefile.haiku test-audio"
echo "   make -f Makefile.haiku test-3d"

# Cleanup
rm -rf "$TEMP_DIR"

echo
echo "🚀 Deployment complete! VeniceDAW ready for native Haiku testing."