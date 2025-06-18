#pragma once

#include "UserConfig.h"
#include <JuceHeader.h>

//==============================================================================
/**
    Multi-format Plugin Host class that manages a chain of audio plugins.

    This class handles:
    - Loading and unloading plugins (VST2, VST3, AU, CLAP)
    - Managing the plugin chain order
    - Processing audio through the plugin chain
    - Plugin parameter management
    - Plugin state saving/loading
*/
class PluginHost {
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
    PluginHost();
    ~PluginHost();

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

    // Plugin scanning - consolidated interface
    void scanForPlugins(bool useCache = true);                        // Main scanning function
    void scanForPlugins(const juce::StringArray &searchPaths);        // Scan specific paths
    void refreshPluginCache();                                        // Force refresh
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
    struct PluginFormatInfo {
        juce::String formatName;
        juce::StringArray fileExtensions;
        juce::StringArray directoryExtensions;
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

    // Scanning state
    juce::Array<juce::File> filesToScan;
    int currentScanIndex = 0;
    std::unique_ptr<juce::Timer> scanningTimer;

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

    // Consolidated plugin scanning
    void scanPluginsInPaths(const juce::StringArray &searchPaths, juce::Array<PluginInfo> &pluginList);
    void startPluginScan();
    void scanNextPlugin();

    // Format-specific processing
    void processPluginFile(const juce::File &pluginFile, juce::AudioPluginFormat *format, juce::Array<PluginInfo> &pluginList);
    void processPluginBundle(const juce::File &bundleFile, juce::AudioPluginFormat *format, juce::Array<PluginInfo> &pluginList);
    juce::Array<PluginFormatInfo> getSupportedFormats() const;
    juce::AudioPluginFormat* getFormatForFile(const juce::File &pluginFile) const;

    // Helper for adding plugins
    void addPluginToList(const juce::PluginDescription &description);

    // Forward declarations
    class PluginScanningTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};
