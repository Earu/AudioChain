# AudioChain - Cross-Platform Audio Processing with Multi-Format Plugin Chain

[![Build AudioChain](https://github.com/YOUR_USERNAME/AudioChain/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/AudioChain/actions/workflows/build.yml)

AudioChain is a JUCE-based application that processes audio input through a customizable plugin chain supporting VST2, VST3, and Audio Unit formats. This enables real-time audio processing and effects for any audio source on your system, supporting both macOS and Windows platforms.

![image](https://github.com/user-attachments/assets/c2374a4a-3fb8-4b78-977c-84ad483ba88d)


## System Requirements

### macOS
- macOS 10.12 or later
- Xcode 12.0 or later
- JUCE 6.0 or later
- Audio hardware with Core Audio support

### Windows
- Windows 10 or later
- Visual Studio 2019/2022 (Community edition is free)
- JUCE 6.0 or later
- WASAPI/DirectSound audio support (built-in)
- ASIO-compatible audio hardware (recommended for low latency)

## Installation

### Prerequisites
1. **CMake** 3.15 or later
2. **C++ Compiler** with C++17 support:
   - **macOS**: Xcode 12.0 or later (with Xcode Command Line Tools)
   - **Windows**: Visual Studio 2019/2022 (Community edition is free)
3. **Git** (for fetching JUCE dependency)
4. Ensure you have VST3 plugins installed on your system

### Building from Source

AudioChain uses CMake for cross-platform building with automatic JUCE dependency management.

#### Quick Start
```bash
# Clone the repository
git clone <your-repo-url>
cd AudioChain

# Initialize and update submodules (includes VST2 SDK)
git submodule update --init --recursive

# Create build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
cmake --build . --config Release
```

#### Platform-Specific Instructions

**macOS:**
```bash
# Initialize submodules first
git submodule update --init --recursive

# Configure for Xcode (optional)
cmake -G "Xcode" ..

# Or build directly with make
cmake ..
make -j$(nproc)
```

**Windows:**
```bash
# Initialize submodules first
git submodule update --init --recursive

# Configure for Visual Studio 2022
cmake -G "Visual Studio 17 2022" -A x64 ..

# Or configure for Visual Studio 2019
cmake -G "Visual Studio 16 2019" -A x64 ..

# Build
cmake --build . --config Release
```

#### Build Options

**Debug Build:**
```bash
cmake --build . --config Debug
```

**Specify Build Type (macOS):**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

The built application will be located in:
- **Windows**: `build/AudioChain_artefacts/Release/AudioChain.exe`
- **macOS**: `build/AudioChain_artefacts/Release/AudioChain.app`

### VST2 Support (Included)

AudioChain supports VST2, VST3, and Audio Unit plugins. **VST2 support is automatically enabled through the included git submodule.**

**VST2 SDK Setup:**
The VST2 SDK is included as a git submodule and will be automatically available when you initialize submodules:
```bash
# VST2 SDK is automatically included when you run:
git submodule update --init --recursive
```

**VST2 Plugin Support:**
- **File Extensions**: `.vst` (both files and bundles on macOS), `.dll` (Windows)
- **Scanning**: VST2 plugins are automatically discovered during recursive directory scanning
- **Auto-Detection**: CMake automatically enables VST2 support when the submodule is initialized
- **Source**: VST2 SDK from https://github.com/R-Tur/VST_SDK_2.4 (included as submodule)

## ASIO SDK Setup (Optional)

AudioChain supports ASIO for low-latency audio on Windows. **In CI/CD builds, the ASIO SDK is automatically downloaded and enabled.** For local development, you can optionally set up ASIO support manually.

### For CI/CD Builds

✅ **Automatic Setup**: The GitHub Actions workflow automatically downloads the ASIO SDK 2.3.3 and enables ASIO support in Windows builds.

### For Local Development (Optional)

To enable ASIO support in your local development environment:

1. Visit [Steinberg Developers](https://www.steinberg.net/developers/)
2. Download the ASIO SDK 2.3
3. Extract the downloaded SDK to `external/asiosdk_2.3.3_2019-06-14/` in your AudioChain project directory
4. CMake will automatically detect and enable ASIO support if the SDK is present
5. Rebuild the project:
   ```bash
   cd build
   cmake ..
   cmake --build . --config Release
   ```

**Note:** ASIO support is optional for local development. AudioChain will work without it using DirectSound and WASAPI drivers on Windows. CMake will automatically configure ASIO support when the SDK is detected.

## Usage

### Setting Up Audio Routing

#### macOS Setup
1. Download and install BlackHole virtual audio driver: https://github.com/ExistentialAudio/BlackHole
2. Set BlackHole as the output device in applications you want to process
4. In AudioChain use the BlackHole input device as the input device and your speakers as the output device
3. Audio will flow: App → BlackHole → AudioChain → Your Speakers

#### Windows Setup
1. Install VB-Audio Virtual Cable or similar virtual audio driver
2. Set the virtual cable as output in applications you want to process
4. In AudioChain use the virtual cable input device as the input device and your speakers as the output device
3. Audio will flow: App → virtual cable → AudioChain → Your Speakers

**Note**: Creating true virtual audio devices requires system-level drivers. AudioChain works best with existing virtual audio solutions like BlackHole (macOS) or VB-Audio Cable (Windows).

### Managing the Plugin Chain
1. **Adding Plugins**: Click "Add Plugin" to browse available plugins
2. **Reordering**: Drag plugin slots to reorder the processing chain
3. **Bypass**: Use the bypass button to enable/disable individual plugins
4. **Editing**: Click "Edit" to open a plugin's native editor interface
5. **Removing**: Click "Remove" to unload a plugin from the chain

### Audio Monitoring
- **Level Meters**: Monitor L/R channel levels in real-time
- **Visual Feedback**: Plugin slots show activity and bypass status with color coding
- **Status Display**: Current device status and performance metrics


## Configuration

### Audio Settings
- Sample Rate: 44.1 kHz (default, configurable)
- Buffer Size: 512 samples (default, configurable)
- Channels: Stereo (2 channels)

### Plugin Paths
Plugins are automatically scanned from standard locations (recursively):

**VST3 Plugins:**
- **macOS**: `/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`, `C:\Program Files (x86)\Common Files\VST3\`

**VST2 Plugins (if VST2 SDK is installed):**
- **macOS**: `/Library/Audio/Plug-Ins/VST/`, `~/Library/Audio/Plug-Ins/VST/`
- **Windows**: `C:\Program Files\Steinberg\VSTPlugins\`, `C:\Program Files (x86)\Steinberg\VSTPlugins\`

**Audio Unit Plugins (macOS only):**
- **macOS**: `/Library/Audio/Plug-Ins/Components/`, `~/Library/Audio/Plug-Ins/Components/`

## Troubleshooting

### Virtual Device Not Appearing
- **AudioChain doesn't create a system-visible virtual device by itself**
- **Recommended solution**: Install BlackHole (macOS) or VB-Audio Cable (Windows)
- **Alternative**: Use AudioChain's system audio monitoring mode
- **For true virtual device**: Requires third-party virtual audio drivers
- **Check**: Console output for virtual device status messages
- **Permissions**: Ensure AudioChain has microphone/audio access permissions

### Plugin Loading Issues
- **No plugins showing in browser**:
  - Check that VST3 plugins are installed in standard locations
  - Look at console output (Debug) for scanning information
  - Try refreshing the plugin browser
  - Verify plugins are 64-bit compatible
- **Plugin scanning locations**:
  - macOS: `/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/VST3/`
  - Windows: `C:\Program Files\Common Files\VST3\`

### Audio Dropouts
- Increase audio buffer size
- Close unnecessary applications
- Check CPU usage in Activity Monitor/Task Manager

### Permission Issues (macOS)
- Grant microphone/audio access in System Preferences > Security & Privacy
- Run with administrator privileges if needed
