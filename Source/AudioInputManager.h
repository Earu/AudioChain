#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

//==============================================================================
/**
    Audio Input Manager class that handles audio input device selection
    and provides a simple interface for getting audio data from the selected device.

    This replaces the complex VirtualAudioDriver approach with a simpler
    user-selectable input device approach.
*/
class AudioInputManager {
  public:
    AudioInputManager();
    ~AudioInputManager();

    // Device selection
    juce::StringArray getAvailableInputDevices();
    juce::StringArray getAvailableOutputDevices();
    bool setInputDevice(const juce::String &deviceName);
    bool setOutputDevice(const juce::String &deviceName);
    juce::String getCurrentInputDevice() const;
    juce::String getCurrentOutputDevice() const;

    // Audio device management
    bool start();
    void stop();
    bool isActive() const { return isRunning; }

    // Audio settings
    void setSampleRate(double sampleRate);
    void setBufferSize(int bufferSize);
    double getSampleRate() const { return currentSampleRate; }
    int getBufferSize() const { return currentBufferSize; }

    // Status and info
    juce::String getStatusString() const;
    bool hasValidInputDevice() const;

    // Audio level monitoring
    float getInputLevel(int channel) const;
    bool hasInputSignal() const;

    // Audio device manager access (for MainComponent to use)
    juce::AudioDeviceManager &getAudioDeviceManager() { return audioDeviceManager; }

    // Audio level updating (called by MainComponent)
    void updateInputLevels(const float *const *inputChannelData, int numInputChannels, int numSamples);

  private:
    juce::AudioDeviceManager audioDeviceManager;

    // Current settings
    juce::String currentInputDeviceName;
    juce::String currentOutputDeviceName;
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;

    // Status
    std::atomic<bool> isRunning{false};
    bool isInitialized = false;

    // Input level monitoring
    static constexpr int numChannels = 2;
    std::array<std::atomic<float>, numChannels> inputLevels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInputManager)
};