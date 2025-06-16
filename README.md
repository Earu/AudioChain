# AudioChain - Cross-Platform Audio Processing with VST3 Plugin Chain

AudioChain is a JUCE-based application that processes audio input through a customizable VST3 plugin chain. This enables real-time audio processing and effects for any audio source on your system, supporting both macOS and Windows platforms.

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
1. Install JUCE Framework from [juce.com](https://juce.com)
2. Ensure you have VST3 plugins installed on your system

### Building from Source
1. Clone or download this repository
2. Open `AudioChain.jucer` in the Projucer
3. Set your JUCE modules path in Projucer
4. Export the project for your platform:
   - **macOS**: Select "Xcode (MacOSX)" target
   - **Windows**: Select "Visual Studio 2022" target
5. Build the project in your IDE

```bash
# Clone the repository
git clone <your-repo-url>
cd AudioChain

# Open in Projucer and export for your platform
# Then build using Xcode (macOS) or Visual Studio (Windows)
```

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
1. **Adding Plugins**: Click "Add Plugin" to browse available VST3 plugins
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
VST3 plugins are automatically scanned from standard locations:
- **macOS**: `/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`, `C:\Program Files (x86)\Common Files\VST3\`

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
