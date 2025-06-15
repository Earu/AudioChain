#include "AudioInputManager.h"

//==============================================================================
AudioInputManager::AudioInputManager()
{
    // Initialize input levels
    for (auto& level : inputLevels)
        level = 0.0f;

    // Don't initialize AudioDeviceManager here - do it lazily when needed
}

AudioInputManager::~AudioInputManager()
{
    stop();
}

//==============================================================================
juce::StringArray AudioInputManager::getAvailableInputDevices()
{
    // Ensure AudioDeviceManager is initialized
    if (!isInitialized)
    {
        juce::String error = audioDeviceManager.initialiseWithDefaultDevices(2, 2);
        if (error.isNotEmpty())
        {
            DBG("Failed to initialize AudioDeviceManager: " + error);
            return juce::StringArray();
        }
        isInitialized = true;
    }

    juce::StringArray devices;

    for (auto* deviceType : audioDeviceManager.getAvailableDeviceTypes())
    {
        if (deviceType != nullptr)
        {
            auto inputDevices = deviceType->getDeviceNames(true); // true for input devices
            for (const auto& device : inputDevices)
            {
                // Check if device name is valid and not already in the list
                if (device.isNotEmpty() && !devices.contains(device))
                {
                    DBG("Found input device: " + device);
                    devices.add(device);
                }
                else if (device.isNotEmpty())
                {
                    DBG("Skipping duplicate input device: " + device);
                }
                else
                {
                    DBG("Skipping empty/invalid device name");
                }
            }
        }
    }

    return devices;
}

juce::StringArray AudioInputManager::getAvailableOutputDevices()
{
    // Ensure AudioDeviceManager is initialized
    if (!isInitialized)
    {
        juce::String error = audioDeviceManager.initialiseWithDefaultDevices(2, 2);
        if (error.isNotEmpty())
        {
            DBG("Failed to initialize AudioDeviceManager: " + error);
            return juce::StringArray();
        }
        isInitialized = true;
    }

    juce::StringArray devices;

    for (auto* deviceType : audioDeviceManager.getAvailableDeviceTypes())
    {
        if (deviceType != nullptr)
        {
            auto outputDevices = deviceType->getDeviceNames(false); // false for output devices
            for (const auto& device : outputDevices)
            {
                // Check if device name is valid and not already in the list
                if (device.isNotEmpty() && !devices.contains(device))
                {
                    DBG("Found output device: " + device);
                    devices.add(device);
                }
                else if (device.isNotEmpty())
                {
                    DBG("Skipping duplicate output device: " + device);
                }
                else
                {
                    DBG("Skipping empty/invalid output device name");
                }
            }
        }
    }

    return devices;
}

bool AudioInputManager::setInputDevice(const juce::String& deviceName)
{
    if (deviceName.isEmpty())
        return false;

    // Stop current device if running
    bool wasRunning = isRunning;
    if (wasRunning)
        stop();

    // First, try to determine the device's capabilities
    int availableInputChannels = 1; // Default to mono

    for (auto* deviceType : audioDeviceManager.getAvailableDeviceTypes())
    {
        if (deviceType != nullptr)
        {
            auto device = deviceType->createDevice(deviceName, deviceName);
            if (device != nullptr)
            {
                availableInputChannels = device->getInputChannelNames().size();
                DBG("Device '" + deviceName + "' has " + juce::String(availableInputChannels) + " input channels");
                break;
            }
        }
    }

    // Configure audio device setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    audioDeviceManager.getAudioDeviceSetup(setup);

    setup.inputDeviceName = deviceName;
    setup.outputDeviceName = currentOutputDeviceName; // Use current output device
    setup.useDefaultInputChannels = false; // We'll set channels manually
    setup.useDefaultOutputChannels = false; // We'll set output channels manually
    setup.inputChannels.clear();

    // Configure input channels based on what the device actually supports
    if (availableInputChannels >= 2)
    {
        // Stereo device - enable both channels
        setup.inputChannels.setRange(0, 2, true);
        DBG("Configuring for stereo input (2 channels)");
    }
    else
    {
        // Mono device - enable only the first channel
        setup.inputChannels.setRange(0, 1, true);
        DBG("Configuring for mono input (1 channel)");
    }

    setup.outputChannels.clear();
    if (!currentOutputDeviceName.isEmpty())
    {
        setup.outputChannels.setRange(0, 2, true); // Enable first 2 output channels if output device is set
    }
    setup.sampleRate = currentSampleRate;
    setup.bufferSize = currentBufferSize;

    DBG("Attempting to set audio device setup:");
    DBG("  Input device: " + setup.inputDeviceName);
    DBG("  Sample rate: " + juce::String(setup.sampleRate));
    DBG("  Buffer size: " + juce::String(setup.bufferSize));
    DBG("  Input channels: " + setup.inputChannels.toString(2));

    juce::String error = audioDeviceManager.setAudioDeviceSetup(setup, true);

    if (error.isEmpty())
    {
        currentInputDeviceName = deviceName;
        isInitialized = true;

        // Get the actual device that was opened
        auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
        if (currentDevice)
        {
            DBG("Successfully set input device: " + deviceName);
            DBG("Actual device opened: " + currentDevice->getName());
            DBG("Device sample rate: " + juce::String(currentDevice->getCurrentSampleRate()));
            DBG("Device buffer size: " + juce::String(currentDevice->getCurrentBufferSizeSamples()));
            DBG("Device input channels: " + juce::String(currentDevice->getActiveInputChannels().toInteger()));
            DBG("Device available input channels: " + juce::String(currentDevice->getInputChannelNames().size()));
        }
        else
        {
            DBG("WARNING: Device setup succeeded but no current device found!");
        }

        // Restart if it was running before
        if (wasRunning)
            start();

        return true;
    }
    else
    {
        DBG("Failed to set input device: " + error);
        return false;
    }
}

juce::String AudioInputManager::getCurrentInputDevice() const
{
    return currentInputDeviceName;
}

bool AudioInputManager::setOutputDevice(const juce::String& deviceName)
{
    if (deviceName.isEmpty())
        return false;

    // Stop current device if running
    bool wasRunning = isRunning;
    if (wasRunning)
        stop();

    // Configure audio device setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    audioDeviceManager.getAudioDeviceSetup(setup);

    setup.inputDeviceName = currentInputDeviceName; // Keep current input device
    setup.outputDeviceName = deviceName;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();
    if (!currentInputDeviceName.isEmpty())
    {
        setup.inputChannels.setRange(0, 2, true); // Enable input channels if input device is set
    }
    setup.outputChannels.clear();
    setup.outputChannels.setRange(0, 2, true); // Enable first 2 output channels
    setup.sampleRate = currentSampleRate;
    setup.bufferSize = currentBufferSize;

    DBG("Attempting to set output device setup:");
    DBG("  Output device: " + setup.outputDeviceName);
    DBG("  Input device: " + setup.inputDeviceName);
    DBG("  Sample rate: " + juce::String(setup.sampleRate));
    DBG("  Buffer size: " + juce::String(setup.bufferSize));
    DBG("  Output channels: " + setup.outputChannels.toString(2));

    juce::String error = audioDeviceManager.setAudioDeviceSetup(setup, true);

    if (error.isEmpty())
    {
        currentOutputDeviceName = deviceName;
        isInitialized = true;

        // Get the actual device that was opened
        auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
        if (currentDevice)
        {
            DBG("Successfully set output device: " + deviceName);
            DBG("Actual device opened: " + currentDevice->getName());
            DBG("Device output channels: " + juce::String(currentDevice->getActiveOutputChannels().toInteger()));
        }
        else
        {
            DBG("WARNING: Output device setup succeeded but no current device found!");
        }

        // Restart if it was running before
        if (wasRunning)
            start();

        return true;
    }
    else
    {
        DBG("Failed to set output device: " + error);
        return false;
    }
}

juce::String AudioInputManager::getCurrentOutputDevice() const
{
    return currentOutputDeviceName;
}

//==============================================================================
bool AudioInputManager::start()
{
    if (isRunning)
        return true;

    if (!isInitialized)
    {
        DBG("AudioInputManager not initialized - no input device selected");
        return false;
    }

    // The audio device is already configured and started by setInputDevice
    // We just need to mark ourselves as running
    isRunning = true;

    DBG("AudioInputManager started with device: " + currentInputDeviceName);
    return true;
}

void AudioInputManager::stop()
{
    if (!isRunning)
        return;

    isRunning = false;

    DBG("AudioInputManager stopped");
}

//==============================================================================
void AudioInputManager::setSampleRate(double sampleRate)
{
    if (sampleRate > 0)
    {
        currentSampleRate = sampleRate;

        // Update device if it's already set
        if (isInitialized && !currentInputDeviceName.isEmpty())
        {
            setInputDevice(currentInputDeviceName);
        }
    }
}

void AudioInputManager::setBufferSize(int bufferSize)
{
    if (bufferSize > 0)
    {
        currentBufferSize = bufferSize;

        // Update device if it's already set
        if (isInitialized && !currentInputDeviceName.isEmpty())
        {
            setInputDevice(currentInputDeviceName);
        }
    }
}

//==============================================================================
juce::String AudioInputManager::getStatusString() const
{
    if (!isRunning)
        return "Stopped";

    if (!isInitialized || currentInputDeviceName.isEmpty())
        return "No input device selected";

    return "Recording from: " + currentInputDeviceName;
}

bool AudioInputManager::hasValidInputDevice() const
{
    return isInitialized && !currentInputDeviceName.isEmpty();
}

//==============================================================================
float AudioInputManager::getInputLevel(int channel) const
{
    if (channel >= 0 && channel < numChannels)
        return inputLevels[channel].load();

    return 0.0f;
}

bool AudioInputManager::hasInputSignal() const
{
    const float threshold = 0.001f; // -60dB roughly

    for (int i = 0; i < numChannels; ++i)
    {
        if (inputLevels[i].load() > threshold)
            return true;
    }

    return false;
}

//==============================================================================
void AudioInputManager::updateInputLevels(const float* const* inputChannelData, int numInputChannels, int numSamples)
{
    for (int channel = 0; channel < juce::jmin(numInputChannels, numChannels); ++channel)
    {
        if (inputChannelData[channel])
        {
            float peak = 0.0f;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                float sampleValue = std::abs(inputChannelData[channel][sample]);
                peak = juce::jmax(peak, sampleValue);
            }

            // Simple peak hold with decay
            float currentLevel = inputLevels[channel].load();
            float newLevel = juce::jmax(peak, currentLevel * 0.98f); // Slow decay

            inputLevels[channel] = newLevel;
        }
    }

    // Handle mono input: duplicate the mono signal to both channels for level display
    if (numInputChannels == 1 && inputChannelData[0])
    {
        float monoLevel = inputLevels[0].load();
        inputLevels[1] = monoLevel; // Duplicate left channel level to right channel
    }
}