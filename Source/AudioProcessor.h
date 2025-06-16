#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Custom Audio Processor class for additional audio processing
    outside of the VST3 plugin chain.

    This handles:
    - Audio level monitoring
    - Basic EQ and filtering
    - Gain control
    - Audio analysis and metering
*/
class AudioProcessor {
  public:
    AudioProcessor();
    ~AudioProcessor();

    // Audio processing
    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void processAudio(juce::AudioBuffer<float> &buffer);
    void releaseResources();

    // Control
    void start();
    void stop();
    bool isActive() const { return isRunning; }

    // Parameters
    void setGain(float gainDb);
    float getGain() const { return gainDb; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return processingEnabled; }

    // Metering
    float getPeakLevel(int channel) const;
    float getRMSLevel(int channel) const;
    void resetMeters();

    // Analysis
    const float *getSpectrumData(int channel) const;
    int getSpectrumSize() const { return fftSize / 2; }

  private:
    // Audio parameters
    std::atomic<float> gainDb{0.0f};
    std::atomic<bool> processingEnabled{true};
    std::atomic<bool> isRunning{false};

    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool isPrepared = false;

    // Metering
    static constexpr int numChannels = 2;
    std::array<std::atomic<float>, numChannels> peakLevels;
    std::array<std::atomic<float>, numChannels> rmsLevels;

    juce::LinearSmoothedValue<float> gainSmoothed;

    // FFT Analysis
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    std::array<juce::dsp::FFT, numChannels> fftObjects;
    std::array<juce::dsp::WindowingFunction<float>, numChannels> windowing;
    std::array<std::array<float, fftSize * 2>, numChannels> fftData;
    std::array<std::array<float, fftSize / 2>, numChannels> spectrumData;
    std::array<int, numChannels> fftIndex;

    // Thread safety
    juce::CriticalSection processingLock;

    // Helper methods
    void updateMeters(const juce::AudioBuffer<float> &buffer);
    void updateSpectrum(const juce::AudioBuffer<float> &buffer);
    void processFFT(int channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioProcessor)
};