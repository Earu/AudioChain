#!/bin/bash

# VST2 SDK Installation Script for AudioChain
# This script sets up VST2 support by:
# 1. Adding VST2 SDK as a git submodule
# 2. Copying VST2 headers to JUCE directory
# 3. Regenerating the project with Projucer

set -e  # Exit on any error

echo "=== AudioChain VST2 SDK Installation Script ==="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the AudioChain project directory
if [ ! -f "AudioChain.jucer" ]; then
    print_error "AudioChain.jucer not found. Please run this script from the AudioChain project root directory."
    exit 1
fi

print_status "Found AudioChain project"

# Check if git is available
if ! command -v git &> /dev/null; then
    print_error "Git is not installed or not in PATH"
    exit 1
fi

# Step 1: Add VST2 SDK as git submodule (if not already added)
print_status "Checking for VST2 SDK submodule..."

if [ -d "external/VST_SDK_2.4" ] && [ -f "external/VST_SDK_2.4/.git" ]; then
    print_warning "VST2 SDK submodule already exists, skipping submodule add"
else
    print_status "Adding VST2 SDK as git submodule..."
    git submodule add https://github.com/R-Tur/VST_SDK_2.4.git external/VST_SDK_2.4
    print_success "VST2 SDK submodule added"
fi

# Initialize and update submodules
print_status "Initializing and updating git submodules..."
git submodule update --init --recursive
print_success "Git submodules updated"

# Verify VST2 SDK files exist
if [ ! -f "external/VST_SDK_2.4/pluginterfaces/vst2.x/aeffect.h" ]; then
    print_error "VST2 SDK files not found in submodule. Please check the submodule."
    exit 1
fi

print_success "VST2 SDK files verified"

# Step 2: Find JUCE installation
print_status "Looking for JUCE installation..."

JUCE_PATH=""
POSSIBLE_JUCE_PATHS=(
    "/Users/$USER/JUCE"
    "/Applications/JUCE"
    "/usr/local/JUCE"
    "/opt/JUCE"
)

for path in "${POSSIBLE_JUCE_PATHS[@]}"; do
    if [ -d "$path/modules/juce_audio_processors" ]; then
        JUCE_PATH="$path"
        break
    fi
done

if [ -z "$JUCE_PATH" ]; then
    print_warning "JUCE installation not found in standard locations."
    read -p "Please enter the path to your JUCE installation: " JUCE_PATH
    
    if [ ! -d "$JUCE_PATH/modules/juce_audio_processors" ]; then
        print_error "Invalid JUCE path. Could not find juce_audio_processors module."
        exit 1
    fi
fi

print_success "Found JUCE installation at: $JUCE_PATH"

# Step 3: Copy VST2 headers to JUCE
VST2_TARGET_DIR="$JUCE_PATH/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst2.x"

print_status "Creating VST2 header directory in JUCE..."
sudo mkdir -p "$VST2_TARGET_DIR"

print_status "Copying VST2 headers to JUCE..."
sudo cp -r external/VST_SDK_2.4/pluginterfaces/vst2.x/* "$VST2_TARGET_DIR/"

# Verify the copy was successful
if [ -f "$VST2_TARGET_DIR/aeffect.h" ]; then
    print_success "VST2 headers copied successfully"
else
    print_error "Failed to copy VST2 headers"
    exit 1
fi

# Step 4: Check for Projucer and regenerate project
print_status "Looking for Projucer..."

PROJUCER_PATH=""
POSSIBLE_PROJUCER_PATHS=(
    "$JUCE_PATH/Projucer.app/Contents/MacOS/Projucer"
    "/Applications/Projucer.app/Contents/MacOS/Projucer"
    "/usr/local/bin/Projucer"
    "projucer"
)

for path in "${POSSIBLE_PROJUCER_PATHS[@]}"; do
    if command -v "$path" &> /dev/null; then
        PROJUCER_PATH="$path"
        break
    fi
done

if [ -z "$PROJUCER_PATH" ]; then
    print_warning "Projucer not found. Please regenerate the project manually."
    print_status "Run: [JUCE_PATH]/Projucer.app/Contents/MacOS/Projucer --resave AudioChain.jucer"
else
    print_status "Found Projucer at: $PROJUCER_PATH"
    print_status "Regenerating AudioChain project..."
    "$PROJUCER_PATH" --resave AudioChain.jucer
    print_success "Project regenerated successfully"
fi

# Step 5: Build test (optional)
echo ""
print_status "VST2 SDK installation complete!"
echo ""
echo "Next steps:"
echo "1. The VST2 SDK has been added as a git submodule in external/VST_SDK_2.4"
echo "2. VST2 headers have been copied to your JUCE installation"
echo "3. Your project has been regenerated (if Projucer was found)"
echo ""
echo "To build with VST2 support:"
echo "  xcodebuild -project Builds/MacOSX/AudioChain.xcodeproj -configuration Debug build"
echo ""
echo "VST2 plugins with .vst extension (files and bundles) will now be detected during plugin scanning."
echo ""

# Optional: Ask if user wants to build now
read -p "Would you like to build the project now to test VST2 support? (y/n): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Building AudioChain with VST2 support..."
    if xcodebuild -project Builds/MacOSX/AudioChain.xcodeproj -configuration Debug build; then
        print_success "Build completed successfully! VST2 support is now active."
    else
        print_error "Build failed. Please check the error messages above."
        exit 1
    fi
fi

print_success "Installation script completed successfully!" 