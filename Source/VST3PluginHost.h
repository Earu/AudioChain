#pragma once

#include "UserConfig.h"
#include <JuceHeader.h>

//==============================================================================
/**
    VST3 Plugin Host class that manages a chain of VST3 plugins.

    This class handles:
    - Loading and unloading VST3 plugins
    - Managing the plugin chain order
    - Processing audio through the plugin chain
    - Plugin parameter management
    - Plugin state saving/loading
*/
class VST3PluginHost {
  public:
    //==============================================================================
    struct PluginInfo {
        juce::String name;
        juce::String manufacturer;
        juce::String version;
        juce::String pluginFormatName;
        juce::String fileOrIdentifier;
        int numInputChannels = 0;
        int numOutputChannels = 0;
        bool isInstrument = false;
        bool hasEditor = false;

        // Store the complete JUCE plugin description for accurate loading
        juce::PluginDescription juceDescription;
        bool hasJuceDescription = false;
    };

    //==============================================================================
    VST3PluginHost();
    ~VST3PluginHost();

    // Audio processing
    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void processAudio(juce::AudioBuffer<float> &buffer);
    void releaseResources();

    // Plugin management
    bool loadPlugin(const juce::String &pluginPath);
    bool loadPlugin(const PluginInfo &pluginInfo);
    void unloadPlugin(int index);
    void clearAllPlugins();

    // Plugin chain management
    void movePlugin(int fromIndex, int toIndex);
    void bypassPlugin(int index, bool shouldBypass);
    bool isPluginBypassed(int index) const;

    // Plugin access
    int getNumPlugins() const { return pluginChain.size(); }
    juce::AudioProcessor *getPlugin(int index);
    const juce::AudioProcessor *getPlugin(int index) const;
    PluginInfo getPluginInfo(int index) const;

    // Plugin scanning
    void scanForPlugins();
    void scanForPlugins(const juce::StringArray &searchPaths);
    const juce::Array<PluginInfo> &getAvailablePlugins() const { return availablePlugins; }

    // Configuration
    void setUserConfig(UserConfig *config) { userConfig = config; }

    // Plugin editors
    juce::AudioProcessorEditor *createEditorForPlugin(int index);
    void closeEditorForPlugin(int index);

    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree &state);

    // Callbacks
    std::function<void()> onPluginChainChanged;
    std::function<void(int, const juce::String &)> onPluginError;

  private:
    //==============================================================================
    struct PluginInstance {
        std::unique_ptr<juce::AudioProcessor> processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor;
        PluginInfo info;
        bool bypassed = false;
        juce::String errorMessage;

        bool isValid() const { return processor != nullptr; }
    };

    //==============================================================================
    juce::OwnedArray<PluginInstance> pluginChain;
    juce::Array<PluginInfo> availablePlugins;

    // Audio format managers
    juce::AudioPluginFormatManager formatManager;

    // Audio processing
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool isPrepared = false;

    // Threading
    juce::CriticalSection pluginLock;

    // Configuration
    UserConfig *userConfig = nullptr;

    // Helper methods
    PluginInfo createPluginInfo(const juce::PluginDescription &description);
    bool validatePlugin(juce::AudioProcessor *processor);
    void initializePlugin(PluginInstance *instance);

    // Plugin scanning
    void scanVST3Plugins();
    void addPluginToList(const juce::PluginDescription &description);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST3PluginHost)
};
