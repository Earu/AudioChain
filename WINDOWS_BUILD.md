# Building AudioChain on Windows

This guide explains how to build and run AudioChain on Windows after the cross-platform updates.

## Prerequisites

1. **Visual Studio 2019 or 2022** (Community edition is free)
   - Install with "Desktop development with C++" workload
   - Includes Windows SDK automatically

2. **JUCE Framework**
   - Make sure JUCE is properly installed and accessible
   - The project uses global JUCE paths

## Building the Project

1. **Generate Visual Studio Project**
   ```
   Open AudioChain.jucer in Projucer
   Click "Save and Open in IDE" or use the Visual Studio 2022 export
   ```

2. **Build in Visual Studio**
   - Open `Builds/VisualStudio2022/AudioChain.sln`
   - Select Debug or Release configuration
   - Build → Build Solution (Ctrl+Shift+B)

## Windows-Specific Features

### Audio Drivers
- **WASAPI**: Modern Windows audio API (recommended)
- **DirectSound**: Legacy support for older systems
- Both are enabled in the JUCE project settings

### VST3 Plugin Locations
The application will scan these directories for VST3 plugins:
- `C:\Program Files\Common Files\VST3\` (system-wide)
- `%APPDATA%\VST3\` (user-specific)

### Microphone Permissions
On Windows 10/11, ensure microphone access is enabled:
1. Go to Settings → Privacy → Microphone
2. Enable "Allow apps to access your microphone"
3. Make sure AudioChain is in the allowed list

## Differences from macOS Version

### Plugin Formats
- **Windows**: VST3 support only
- **macOS**: VST3 + AudioUnit support

### Bundle Structure
- **Windows**: VST3 plugins use `Contents/x86_64-win/` structure
- **macOS**: VST3 plugins use `Contents/MacOS/` structure

### Audio APIs
- **Windows**: WASAPI, DirectSound
- **macOS**: Core Audio

## Troubleshooting

### No Audio Devices Found
- Check Windows audio service is running
- Verify microphone permissions
- Try different audio drivers (WASAPI vs DirectSound)

### VST3 Plugins Not Loading
- Verify VST3 plugins are installed in correct directories
- Check plugin compatibility (64-bit only)
- Look at debug output for scanning errors

### Build Errors
- Ensure all JUCE modules are found
- Check Visual Studio platform toolset compatibility
- Verify Windows SDK version matches project requirements

## Performance Notes

- WASAPI provides lower latency than DirectSound
- Use Release builds for better real-time performance
- Consider buffer size settings for your audio interface

## Cross-Platform Compatibility

The codebase now includes platform-specific conditional compilation:
- Microphone permission messages
- Plugin format initialization
- Plugin directory scanning
- Bundle structure validation

All core audio processing remains platform-agnostic thanks to JUCE. 