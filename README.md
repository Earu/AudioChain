# AudioChain - Virtual Audio Device with VST3 Plugin Chain

AudioChain is a JUCE-based application that creates a virtual audio device allowing other applications to route audio through a customizable VST3 plugin chain. This enables real-time audio processing and effects for any audio source on your system.

## Features

### 🎛️ Virtual Audio Device
- Creates a virtual audio device that appears as a speaker in your system
- Routes audio from other applications through the VST3 plugin chain
- Real-time audio processing with low latency
- Cross-platform support (macOS and Windows)

### 🔌 VST3 Plugin Host
- Load and manage VST3 plugins in a chain
- Drag-and-drop plugin reordering
- Individual plugin bypass controls
- Plugin editor window support
- State saving and loading
- Automatic plugin scanning

### 🎚️ Audio Processing
- Built-in gain control and metering
- Real-time level meters for L/R channels
- Spectrum analyzer for audio visualization
- Professional audio processing pipeline

### 🖥️ User Interface
- Modern, intuitive plugin chain interface
- Visual plugin slot representation
- Plugin browser with search capabilities
- Real-time audio level monitoring
- Responsive layout design

## System Requirements

### macOS
- macOS 10.12 or later
- Xcode 12.0 or later
- JUCE 6.0 or later
- Audio hardware with Core Audio support

### Windows
- Windows 10 or later
- Visual Studio 2019 or later
- JUCE 6.0 or later
- ASIO-compatible audio hardware (recommended)

## Installation

### Prerequisites
1. Install JUCE Framework from [juce.com](https://juce.com)
2. Ensure you have VST3 plugins installed on your system

### Building from Source
1. Clone or download this repository
2. Open `AudioChain.jucer` in the Projucer
3. Set your JUCE modules path in Projucer
4. Export the project for your platform (Xcode/Visual Studio)
5. Build the project in your IDE

```bash
# Clone the repository
git clone <your-repo-url>
cd AudioChain

# Open in Projucer and export for your platform
# Then build using Xcode (macOS) or Visual Studio (Windows)
```

## Usage

### Starting the Virtual Device
1. Launch AudioChain
2. Click "Start Virtual Device" in the main interface
3. The virtual device "AudioChain Virtual Device" will appear in your system's audio devices

### Setting Up Audio Routing

#### macOS Setup (Recommended)
**Option 1: Using BlackHole (Recommended)**
1. Download and install BlackHole virtual audio driver: https://github.com/ExistentialAudio/BlackHole
2. Set BlackHole as the output device in applications you want to process
3. Set your speakers/headphones as AudioChain's output device
4. Start AudioChain virtual device
5. Audio will flow: App → BlackHole → AudioChain → Your Speakers

**Option 2: System Audio Monitoring**
1. Start AudioChain virtual device (monitors system audio output)
2. Configure your system output device normally
3. AudioChain will process audio passing through the system

#### Windows Setup
1. Install VB-Audio Virtual Cable or similar virtual audio driver
2. Set the virtual cable as output in applications you want to process
3. Set your speakers as AudioChain's output device
4. Start AudioChain virtual device

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

## Architecture

### Core Components

#### VirtualAudioDriver
- Platform-specific virtual audio device implementation
- Handles audio routing from system to application
- Ring buffer management for thread-safe audio transfer

#### VST3PluginHost
- VST3 plugin loading and management
- Plugin chain processing
- State serialization and parameter management

#### AudioProcessor
- Additional audio processing capabilities
- Gain control and audio analysis
- Real-time metering and spectrum analysis

#### PluginChainComponent
- User interface for plugin management
- Visual plugin chain representation
- Plugin browser and editor integration

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

## Development

### Project Structure
```
AudioChain/
├── Source/
│   ├── Main.cpp                     # Application entry point
│   ├── MainComponent.h/.cpp         # Main UI component
│   ├── VirtualAudioDriver.h/.cpp    # Virtual device implementation
│   ├── VST3PluginHost.h/.cpp        # Plugin hosting system
│   ├── AudioProcessor.h/.cpp        # Audio processing engine
│   └── PluginChainComponent.h/.cpp  # Plugin chain UI
├── AudioChain.jucer                 # JUCE project file
└── README.md                        # This file
```

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Key APIs Used
- **JUCE Audio Framework**: Core audio processing and device management
- **VST3 SDK**: Plugin hosting and management
- **Core Audio** (macOS): Low-level audio device creation
- **WASAPI** (Windows): Audio device interface

## Technical Notes

### Virtual Audio Device Implementation
The virtual audio device implementation varies by platform:

- **macOS**: Uses Core Audio's AudioUnit framework to create a virtual device that routes audio through the application
- **Windows**: Implements a WASAPI-based virtual audio endpoint (requires elevated privileges)

### Thread Safety
- All audio processing occurs on dedicated audio threads
- UI updates are synchronized with the message thread
- Plugin loading/unloading is thread-safe with proper locking

### Performance Considerations
- Optimized for real-time audio processing
- Minimal memory allocations in audio callbacks
- Efficient plugin chain processing with bypass capabilities

## License

This project is provided as an educational example. Please ensure you have appropriate licenses for any VST3 plugins you use with this software.

## Support

For issues and questions:
1. Check the troubleshooting section above
2. Review JUCE documentation at [docs.juce.com](https://docs.juce.com)
3. Consult VST3 SDK documentation for plugin-related issues

---

**Note**: This is a complex audio application that interfaces with system-level audio APIs. Some features may require administrative privileges or specific system configurations. Always test thoroughly in your environment before use in production scenarios. 