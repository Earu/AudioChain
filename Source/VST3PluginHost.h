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

        // Architecture information
        bool is64Bit = true;
        bool isCompatible = true;
        juce::String architectureString;

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
    void scanForPluginsAsync();
    void scanForPluginsSync();  // Force synchronous scan
    void refreshPluginCache();
    bool isPluginCacheValid() const { return pluginCacheValid; }
    bool isScanning() const { return isCurrentlyScanning; }
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
    std::function<void()> onPluginScanComplete;

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

    // Plugin cache and scanning state
    bool pluginCacheValid = false;
    bool isCurrentlyScanning = false;
    std::unique_ptr<juce::Thread> scanningThread;

    // Main thread scanning (timer-based)
    std::unique_ptr<juce::Timer> scanningTimer;
    juce::Array<juce::File> filesToScan;
    int currentScanIndex = 0;

    // Helper methods
    PluginInfo createPluginInfo(const juce::PluginDescription &description);
    bool validatePlugin(juce::AudioProcessor *processor);
    void initializePlugin(PluginInstance *instance);

    // Architecture detection
    bool isHostArchitecture64Bit() const;
    bool isPluginArchitectureCompatible(const juce::File &pluginFile) const;
    juce::String getPluginArchitecture(const juce::File &pluginFile) const;
    juce::String analyzeWindowsPEArchitecture(const juce::File &pluginFile) const;
    juce::String analyzeMacBinaryArchitecture(const juce::File &binaryFile) const;

    // Plugin scanning (internal)
    void performPluginScan();
    void scanPluginsInPaths(const juce::StringArray &searchPaths, juce::Array<PluginInfo> &pluginList);
    void processVST3File(const juce::File &vstFile, juce::AudioPluginFormat *vst3Format, juce::Array<PluginInfo> &pluginList);
    void processVST3Bundle(const juce::File &vstBundle, juce::AudioPluginFormat *vst3Format, juce::Array<PluginInfo> &pluginList);

    // Plugin scanning
    void addPluginToList(const juce::PluginDescription &description);

    // Timer-based scanning (main thread)
    void startMainThreadScan();
    void scanNextPlugin();

    // Forward declarations
    class PluginScanningThread;
    class PluginScanningTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VST3PluginHost)
};
