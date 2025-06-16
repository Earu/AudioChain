#include "AudioProcessor.h"

//==============================================================================
AudioProcessor::AudioProcessor()
    : fftObjects{juce::dsp::FFT(fftOrder), juce::dsp::FFT(fftOrder)},
      windowing{juce::dsp::WindowingFunction<float>(fftSize, juce::dsp::WindowingFunction<float>::hann),
                juce::dsp::WindowingFunction<float>(fftSize, juce::dsp::WindowingFunction<float>::hann)} {
    // Initialize meters
    for (int i = 0; i < numChannels; ++i) {
        peakLevels[i] = 0.0f;
        rmsLevels[i] = 0.0f;
        fftIndex[i] = 0;

        // Clear FFT data
        std::fill(fftData[i].begin(), fftData[i].end(), 0.0f);
        std::fill(spectrumData[i].begin(), spectrumData[i].end(), 0.0f);
    }
}

AudioProcessor::~AudioProcessor() { stop(); }

//==============================================================================
void AudioProcessor::prepareToPlay(int samplesPerBlock, double sampleRate) {
    juce::ScopedLock lock(processingLock);

    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Prepare gain smoothing
    gainSmoothed.reset(sampleRate, 0.05); // 50ms smoothing
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gainDb.load()));

    // Reset meters and analysis
    resetMeters();
    for (int i = 0; i < numChannels; ++i) {
        fftIndex[i] = 0;
        std::fill(fftData[i].begin(), fftData[i].end(), 0.0f);
        std::fill(spectrumData[i].begin(), spectrumData[i].end(), 0.0f);
    }

    isPrepared = true;
}

void AudioProcessor::processAudio(juce::AudioBuffer<float> &buffer) {
    juce::ScopedLock lock(processingLock);

    if (!isPrepared || !isRunning || !processingEnabled)
        return;

    // Update gain smoothing target
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(gainDb.load()));

    // Apply gain
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto *channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] *= gainSmoothed.getNextValue();
        }
    }

    // Update meters and spectrum
    updateMeters(buffer);
    updateSpectrum(buffer);
}

void AudioProcessor::releaseResources() { isPrepared = false; }

//==============================================================================
void AudioProcessor::start() { isRunning = true; }

void AudioProcessor::stop() { isRunning = false; }

//==============================================================================
void AudioProcessor::setGain(float gainDb_) { gainDb = gainDb_; }

void AudioProcessor::setEnabled(bool enabled) { processingEnabled = enabled; }

//==============================================================================
float AudioProcessor::getPeakLevel(int channel) const {
    if (channel >= 0 && channel < numChannels)
        return peakLevels[channel].load();
    return 0.0f;
}

float AudioProcessor::getRMSLevel(int channel) const {
    if (channel >= 0 && channel < numChannels)
        return rmsLevels[channel].load();
    return 0.0f;
}

void AudioProcessor::resetMeters() {
    for (int i = 0; i < numChannels; ++i) {
        peakLevels[i] = 0.0f;
        rmsLevels[i] = 0.0f;
    }
}

//==============================================================================
const float *AudioProcessor::getSpectrumData(int channel) const {
    if (channel >= 0 && channel < numChannels)
        return spectrumData[channel].data();
    return nullptr;
}

//==============================================================================
void AudioProcessor::updateMeters(const juce::AudioBuffer<float> &buffer) {
    for (int channel = 0; channel < std::min(buffer.getNumChannels(), numChannels); ++channel) {
        auto *channelData = buffer.getReadPointer(channel);

        // Calculate peak level
        float peak = 0.0f;
        float rmsSum = 0.0f;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float sampleValue = std::abs(channelData[sample]);
            peak = juce::jmax(peak, sampleValue);
            rmsSum += sampleValue * sampleValue;
        }

        // Update peak level with decay
        float currentPeak = peakLevels[channel].load();
        float newPeak = juce::jmax(peak, currentPeak * 0.95f); // Decay factor
        peakLevels[channel] = newPeak;

        // Calculate and update RMS level
        float rms = std::sqrt(rmsSum / buffer.getNumSamples());
        rmsLevels[channel] = rms;
    }
}

void AudioProcessor::updateSpectrum(const juce::AudioBuffer<float> &buffer) {
    for (int channel = 0; channel < std::min(buffer.getNumChannels(), numChannels); ++channel) {
        auto *channelData = buffer.getReadPointer(channel);

        // Copy audio data into FFT buffer
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            if (fftIndex[channel] < fftSize) {
                fftData[channel][fftIndex[channel]] = channelData[sample];
                fftIndex[channel]++;
            }
        }

        // Process FFT when buffer is full
        if (fftIndex[channel] >= fftSize) {
            processFFT(channel);
            fftIndex[channel] = 0;
        }
    }
}

void AudioProcessor::processFFT(int channel) {
    if (channel < 0 || channel >= numChannels)
        return;

    // Apply windowing
    windowing[channel].multiplyWithWindowingTable(fftData[channel].data(), fftSize);

    // Perform FFT
    fftObjects[channel].performFrequencyOnlyForwardTransform(fftData[channel].data());

    // Convert to spectrum data (magnitude)
    for (int i = 0; i < fftSize / 2; ++i) {
        float magnitude = fftData[channel][i];

        // Convert to dB and smooth
        float dB = juce::jmax(-100.0f, juce::Decibels::gainToDecibels(magnitude));
        spectrumData[channel][i] = spectrumData[channel][i] * 0.8f + dB * 0.2f; // Smoothing
    }
}