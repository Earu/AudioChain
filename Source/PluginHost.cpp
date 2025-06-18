#include "PluginHost.h"
#include "UserConfig.h"
#include <vector>
#include <cmath>
#include <memory>

//==============================================================================
// Main Thread Scanning Timer
class PluginHost::PluginScanningTimer : public juce::Timer
{
public:
    PluginScanningTimer(PluginHost& host) : pluginHost(host) {}

    void timerCallback() override
    {
        pluginHost.scanNextPlugin();
    }

private:
    PluginHost& pluginHost;
};

//==============================================================================
PluginHost::PluginHost() {
    // Initialize format manager with supported formats
    formatManager.addFormat(new juce::VST3PluginFormat());
    
#if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
#endif

// Future plugin formats can be enabled when available
#if JUCE_PLUGINHOST_VST && JUCE_PLUGINHOST_VST_LEGACY  
    formatManager.addFormat(new juce::VSTPluginFormat());
#endif

// CLAP support (when available in JUCE)
#if 0  // Enable when CLAP support is added to JUCE
    formatManager.addFormat(new juce::CLAPPluginFormat());
#endif

    // Debug: Check what formats are available
    DBG("Format manager initialized with " + juce::String(formatManager.getNumFormats()) + " formats:");
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto *format = formatManager.getFormat(i);
        if (format != nullptr) {
            DBG("  Format " + juce::String(i) + ": " + format->getName());
        }
    }

    // Don't scan on initialization - plugins will be scanned on first access
    // Use scanForPlugins() or scanForPluginsAsync() when needed
}

PluginHost::~PluginHost() { 
    clearAllPlugins(); 
}

//==============================================================================
void PluginHost::prepareToPlay(int samplesPerBlock, double sampleRate) {
    juce::ScopedLock lock(pluginLock);

    currentBlockSize = samplesPerBlock;
    currentSampleRate = sampleRate;

    // Prepare all plugins in the chain
    for (auto *plugin : pluginChain) {
        if (plugin->isValid()) {
            plugin->processor->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }

    isPrepared = true;
}

void PluginHost::processAudio(juce::AudioBuffer<float> &buffer) {
    juce::ScopedLock lock(pluginLock);

    if (!isPrepared)
        return;

    // Process through each plugin in the chain
    for (auto *plugin : pluginChain) {
        if (plugin->isValid() && !plugin->bypassed) {
            try {
                // Create MIDI buffer (empty for now)
                juce::MidiBuffer midiBuffer;

                // Process the audio
                plugin->processor->processBlock(buffer, midiBuffer);
            } catch (const std::exception &e) {
                // Handle plugin processing errors
                plugin->errorMessage = "Processing error: " + juce::String(e.what());
                plugin->bypassed = true; // Bypass plugin on error

                if (onPluginError) {
                    onPluginError(pluginChain.indexOf(plugin), plugin->errorMessage);
                }
            }
        }
    }
}

void PluginHost::releaseResources() {
    juce::ScopedLock lock(pluginLock);

    for (auto *plugin : pluginChain) {
        if (plugin->isValid()) {
            plugin->processor->releaseResources();
        }
    }

    isPrepared = false;
}

//==============================================================================
bool PluginHost::loadPlugin(const juce::String &pluginPath) {
    // Find plugin info by path
    for (const auto &pluginInfo : availablePlugins) {
        if (pluginInfo.fileOrIdentifier == pluginPath) {
            return loadPlugin(pluginInfo);
        }
    }
    return false;
}

bool PluginHost::loadPlugin(const PluginInfo &pluginInfo) {
    juce::ScopedLock lock(pluginLock);

    // Check architecture compatibility before attempting to load
    if (!pluginInfo.isCompatible) {
        DBG("Attempting to load incompatible plugin: " + pluginInfo.name + " (" + pluginInfo.architectureString + ")");

        if (onPluginError) {
            onPluginError(-1, "Plugin architecture (" + pluginInfo.architectureString +
                         ") is incompatible with host (" + (isHostArchitecture64Bit() ? "x64" : "x86") + ")");
        }
        return false;
    }

    // Check if trying to load an instrument (effects only)
    if (pluginInfo.isInstrument) {
        DBG("Attempting to load instrument plugin: " + pluginInfo.name);

        if (onPluginError) {
            onPluginError(-1, "Cannot load instrument '" + pluginInfo.name + "'");
        }
        return false;
    }

    // Use the stored JUCE description if available, otherwise create one manually
    juce::PluginDescription description;
    if (pluginInfo.hasJuceDescription) {
        description = pluginInfo.juceDescription;
        DBG("Using stored JUCE description for: " + pluginInfo.name);
    } else {
        // Fallback: create description manually
        description.name = pluginInfo.name;
        description.manufacturerName = pluginInfo.manufacturer;
        description.version = pluginInfo.version;
        description.pluginFormatName = pluginInfo.pluginFormatName;
        description.fileOrIdentifier = pluginInfo.fileOrIdentifier;
        description.numInputChannels = pluginInfo.numInputChannels;
        description.numOutputChannels = pluginInfo.numOutputChannels;
        description.isInstrument = pluginInfo.isInstrument;
        description.hasSharedContainer = false;
        DBG("Using manual description for: " + pluginInfo.name);
    }

    // Create plugin instance
    juce::String errorMessage;
    std::unique_ptr<juce::AudioProcessor> processor(
        formatManager.createPluginInstance(description, currentSampleRate, currentBlockSize, errorMessage));

    if (!processor) {
        DBG("Failed to create plugin instance for: " + pluginInfo.name);
        DBG("Error message: " + errorMessage);
        DBG("Plugin path: " + pluginInfo.fileOrIdentifier);
        DBG("Plugin format: " + pluginInfo.pluginFormatName);

        if (onPluginError) {
            onPluginError(-1, "Failed to load plugin: " + errorMessage);
        }
        return false;
    }

    // Validate plugin
    if (!validatePlugin(processor.get())) {
        if (onPluginError) {
            onPluginError(-1, "Plugin validation failed");
        }
        return false;
    }

    // Create plugin instance wrapper
    auto *instance = new PluginInstance();
    instance->processor = std::move(processor);
    instance->info = pluginInfo;
    instance->bypassed = false;

    // Initialize the plugin
    initializePlugin(instance);

    // Add to chain
    pluginChain.add(instance);

    // Prepare plugin if we're already prepared
    if (isPrepared) {
        instance->processor->prepareToPlay(currentSampleRate, currentBlockSize);
    }

    // Notify listeners
    if (onPluginChainChanged) {
        onPluginChainChanged();
    }

    return true;
}

void PluginHost::unloadPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        // Close editor if open
        closeEditorForPlugin(index);

        // Remove from chain
        pluginChain.remove(index);

        // Notify listeners
        if (onPluginChainChanged) {
            onPluginChainChanged();
        }
    }
}

void PluginHost::clearAllPlugins() {
    juce::ScopedLock lock(pluginLock);

    // Close all editors
    for (int i = 0; i < pluginChain.size(); ++i) {
        closeEditorForPlugin(i);
    }

    // Clear chain
    pluginChain.clear();

    // Notify listeners
    if (onPluginChainChanged) {
        onPluginChainChanged();
    }
}

//==============================================================================
void PluginHost::movePlugin(int fromIndex, int toIndex) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(fromIndex, pluginChain.size()) &&
        juce::isPositiveAndBelow(toIndex, pluginChain.size()) && fromIndex != toIndex) {
        pluginChain.move(fromIndex, toIndex);

        if (onPluginChainChanged) {
            onPluginChainChanged();
        }
    }
}

void PluginHost::bypassPlugin(int index, bool shouldBypass) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        pluginChain[index]->bypassed = shouldBypass;
    }
}

bool PluginHost::isPluginBypassed(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->bypassed;
    }
    return false;
}

//==============================================================================
juce::AudioProcessor *PluginHost::getPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

const juce::AudioProcessor *PluginHost::getPlugin(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

PluginHost::PluginInfo PluginHost::getPluginInfo(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->info;
    }
    return {};
}

//==============================================================================
void PluginHost::scanForPlugins(bool useCache) {
    // Check if we should use cache and have valid cached plugins
    if (useCache && pluginCacheValid && !availablePlugins.isEmpty()) {
        DBG("Using cached plugin list (" + juce::String(availablePlugins.size()) + " plugins)");
        return;
    }

    // Start scanning
    if (isCurrentlyScanning) {
        DBG("Already scanning plugins, ignoring request");
        return;
    }

    refreshPluginCache();
}

juce::AudioProcessorEditor *PluginHost::createEditorForPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        auto *instance = pluginChain[index];
        if (instance->isValid() && !instance->editor) {
            instance->editor.reset(instance->processor->createEditor());
            return instance->editor.get();
        }
    }
    return nullptr;
}

void PluginHost::closeEditorForPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        pluginChain[index]->editor.reset();
    }
}

//==============================================================================
juce::ValueTree PluginHost::getState() const {
    juce::ScopedLock lock(pluginLock);

    juce::ValueTree state("PluginChain");

    for (int i = 0; i < pluginChain.size(); ++i) {
        auto *instance = pluginChain[i];
        if (instance->isValid()) {
            juce::ValueTree pluginState("Plugin");
            pluginState.setProperty("name", instance->info.name, nullptr);
            pluginState.setProperty("manufacturer", instance->info.manufacturer, nullptr);
            pluginState.setProperty("version", instance->info.version, nullptr);
            pluginState.setProperty("fileOrIdentifier", instance->info.fileOrIdentifier, nullptr);
            pluginState.setProperty("bypassed", instance->bypassed, nullptr);

            // Save plugin internal state
            juce::MemoryBlock stateBlock;
            instance->processor->getStateInformation(stateBlock);
            pluginState.setProperty("state", stateBlock.toBase64Encoding(), nullptr);

            state.appendChild(pluginState, nullptr);
        }
    }

    return state;
}

void PluginHost::setState(const juce::ValueTree &state) {
    if (!state.hasType("PluginChain"))
        return;

    clearAllPlugins();

    for (int i = 0; i < state.getNumChildren(); ++i) {
        juce::ValueTree pluginState = state.getChild(i);
        if (pluginState.hasType("Plugin")) {
            PluginInfo info;
            info.name = pluginState.getProperty("name", "");
            info.manufacturer = pluginState.getProperty("manufacturer", "");
            info.version = pluginState.getProperty("version", "");
            info.fileOrIdentifier = pluginState.getProperty("fileOrIdentifier", "");

            if (loadPlugin(info)) {
                int pluginIndex = pluginChain.size() - 1;

                // Restore bypass state
                bool bypassed = pluginState.getProperty("bypassed", false);
                bypassPlugin(pluginIndex, bypassed);

                // Restore plugin internal state
                juce::String stateString = pluginState.getProperty("state", "");
                if (stateString.isNotEmpty()) {
                    juce::MemoryBlock stateBlock;
                    stateBlock.fromBase64Encoding(stateString);

                    if (auto *processor = getPlugin(pluginIndex)) {
                        processor->setStateInformation(stateBlock.getData(), (int)stateBlock.getSize());
                    }
                }
            }
        }
    }
}

void PluginHost::processPluginFile(const juce::File &pluginFile, juce::AudioPluginFormat *format, juce::Array<PluginInfo> &pluginList) {
    DBG("  Found plugin file: " + pluginFile.getFullPathName() + " (Format: " + format->getName() + ")");

    // Check architecture compatibility FIRST, before JUCE tries to load it
    juce::String architecture = getPluginArchitecture(pluginFile);
    bool isCompatible = isPluginArchitectureCompatible(pluginFile);

    DBG("    Plugin architecture: " + architecture + ", Compatible: " + (isCompatible ? "Yes" : "No"));

    // Skip incompatible plugins entirely
    if (!isCompatible) {
        DBG("    Skipped incompatible plugin: " + pluginFile.getFileNameWithoutExtension() +
            " (" + architecture + " vs host " + (isHostArchitecture64Bit() ? "x64" : "x86") + ")");
        return;
    }

    // Try to get proper plugin description using JUCE (only for compatible plugins)
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile(descriptions, pluginFile.getFullPathName());

    bool foundDescription = descriptions.size() > 0 && descriptions[0] != nullptr;
    if (foundDescription) {
        DBG("    Successfully read plugin description");
    } else {
        DBG("    Could not read plugin description, using fallback");
    }

    // Create plugin info from description or fallback
    PluginInfo info;
    if (foundDescription) {
        auto *description = descriptions[0]; // Use first description
        info.name = description->name.isNotEmpty() ? description->name
                                                   : pluginFile.getFileNameWithoutExtension();
        info.manufacturer =
            description->manufacturerName.isNotEmpty() ? description->manufacturerName : "Unknown";
        info.version = description->version.isNotEmpty() ? description->version : "1.0";
        info.pluginFormatName = description->pluginFormatName;
        info.fileOrIdentifier = description->fileOrIdentifier;
        info.numInputChannels = description->numInputChannels;
        info.numOutputChannels = description->numOutputChannels;
        info.isInstrument = description->isInstrument;
        info.hasEditor = description->hasSharedContainer;

        // Store the complete JUCE description for accurate loading
        info.juceDescription = *description;
        info.hasJuceDescription = true;
    } else {
        // Fallback to basic info - assume it's an effect if we can't determine
        info.name = pluginFile.getFileNameWithoutExtension();
        info.manufacturer = "Unknown";
        info.version = "1.0";
        info.pluginFormatName = format->getName();
        info.fileOrIdentifier = pluginFile.getFullPathName();
        info.numInputChannels = 2;
        info.numOutputChannels = 2;
        info.isInstrument = false; // Assume effect when unknown
        info.hasEditor = true;
        info.hasJuceDescription = false;
    }

    // Skip instrument plugins (effects only)
    if (info.isInstrument) {
        DBG("    Skipped instrument plugin: " + info.name + " (effects only)");
        return;
    }

    // Set architecture information
    info.architectureString = architecture;
    info.is64Bit = architecture.containsIgnoreCase("64") || architecture.containsIgnoreCase("x64");
    info.isCompatible = isCompatible;

    // Add compatible effect to the list
    pluginList.add(info);
    DBG("    Added effect: " + info.name + " by " + info.manufacturer);
}

void PluginHost::processPluginBundle(const juce::File &bundleFile, juce::AudioPluginFormat *format, juce::Array<PluginInfo> &pluginList) {
    DBG("  Found plugin bundle: " + bundleFile.getFullPathName() + " (Format: " + format->getName() + ")");

    // Validate bundle structure based on format
    bool validBundle = false;
    if (format->getName().containsIgnoreCase("VST3")) {
        auto contentsDir = bundleFile.getChildFile("Contents");
#if JUCE_MAC
        auto macOSDir = contentsDir.getChildFile("MacOS");
        validBundle = contentsDir.exists() && macOSDir.exists();
#elif JUCE_WINDOWS
        auto x64Dir = contentsDir.getChildFile("x86_64-win");
        validBundle = contentsDir.exists() && x64Dir.exists();
#endif
    } else if (format->getName().containsIgnoreCase("AudioUnit")) {
        // AU bundles have different structure - assume valid if it's a directory
        validBundle = bundleFile.isDirectory();
    } else {
        // For other formats, assume valid if it's a directory
        validBundle = bundleFile.isDirectory();
    }

    if (!validBundle) {
        DBG("    Invalid bundle structure for format " + format->getName());
        return;
    }

    DBG("    Valid bundle structure");

    // Check architecture compatibility FIRST, before JUCE tries to load it
    juce::String architecture = getPluginArchitecture(bundleFile);
    bool isCompatible = isPluginArchitectureCompatible(bundleFile);

    DBG("    Plugin architecture: " + architecture + ", Compatible: " + (isCompatible ? "Yes" : "No"));

    // Skip incompatible plugins entirely
    if (!isCompatible) {
        DBG("    Skipped incompatible plugin: " + bundleFile.getFileNameWithoutExtension() +
            " (" + architecture + " vs host " + (isHostArchitecture64Bit() ? "x64" : "x86") + ")");
        return;
    }

    // Try to get proper plugin description using JUCE (only for compatible plugins)
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile(descriptions, bundleFile.getFullPathName());

    bool foundDescription = descriptions.size() > 0 && descriptions[0] != nullptr;
    if (foundDescription) {
        DBG("    Successfully read plugin description");
    } else {
        DBG("    Could not read plugin description, using fallback");
    }

    // Create plugin info from description or fallback
    PluginInfo info;
    if (foundDescription) {
        auto *description = descriptions[0]; // Use first description
        info.name = description->name.isNotEmpty() ? description->name
                                                   : bundleFile.getFileNameWithoutExtension();
        info.manufacturer =
            description->manufacturerName.isNotEmpty() ? description->manufacturerName : "Unknown";
        info.version = description->version.isNotEmpty() ? description->version : "1.0";
        info.pluginFormatName = description->pluginFormatName;
        info.fileOrIdentifier = description->fileOrIdentifier;
        info.numInputChannels = description->numInputChannels;
        info.numOutputChannels = description->numOutputChannels;
        info.isInstrument = description->isInstrument;
        info.hasEditor = description->hasSharedContainer;

        // Store the complete JUCE description for accurate loading
        info.juceDescription = *description;
        info.hasJuceDescription = true;
    } else {
        // Fallback to basic info - assume it's an effect if we can't determine
        info.name = bundleFile.getFileNameWithoutExtension();
        info.manufacturer = "Unknown";
        info.version = "1.0";
        info.pluginFormatName = format->getName();
        info.fileOrIdentifier = bundleFile.getFullPathName();
        info.numInputChannels = 2;
        info.numOutputChannels = 2;
        info.isInstrument = false; // Assume effect when unknown
        info.hasEditor = true;
        info.hasJuceDescription = false;
    }

    // Skip instrument plugins (effects only)
    if (info.isInstrument) {
        DBG("    Skipped instrument plugin: " + info.name + " (effects only)");
        return;
    }

    // Set architecture information
    info.architectureString = architecture;
    info.is64Bit = architecture.containsIgnoreCase("64") || architecture.containsIgnoreCase("x64");
    info.isCompatible = isCompatible;

    // Add compatible effect to the list
    pluginList.add(info);
    DBG("    Added effect: " + info.name + " by " + info.manufacturer);
}

//==============================================================================
PluginHost::PluginInfo PluginHost::createPluginInfo(const juce::PluginDescription &description) {
    PluginInfo info;
    info.name = description.name;
    info.manufacturer = description.manufacturerName;
    info.version = description.version;
    info.pluginFormatName = description.pluginFormatName;
    info.fileOrIdentifier = description.fileOrIdentifier;
    info.numInputChannels = description.numInputChannels;
    info.numOutputChannels = description.numOutputChannels;
    info.isInstrument = description.isInstrument;
    info.hasEditor = description.hasSharedContainer; // This might need adjustment

    // Set architecture information
    juce::File pluginFile(description.fileOrIdentifier);
    info.architectureString = getPluginArchitecture(pluginFile);
    info.is64Bit = info.architectureString.containsIgnoreCase("64") || info.architectureString.containsIgnoreCase("x64");
    info.isCompatible = isPluginArchitectureCompatible(pluginFile);

    // Store the complete JUCE description
    info.juceDescription = description;
    info.hasJuceDescription = true;

    return info;
}

bool PluginHost::validatePlugin(juce::AudioProcessor *processor) {
    if (!processor)
        return false;

    // Basic validation checks
    if (processor->getName().isEmpty())
        return false;

    // Check if plugin accepts stereo input/output
    if (processor->getMainBusNumInputChannels() > 2 || processor->getMainBusNumOutputChannels() > 2) {
        // For now, we only support mono/stereo plugins
        return false;
    }

    return true;
}

void PluginHost::initializePlugin(PluginInstance *instance) {
    if (!instance || !instance->isValid())
        return;

    // Set up bus layouts for stereo processing
    auto *processor = instance->processor.get();

    // Configure input bus
    if (processor->getBusCount(true) > 0) {
        processor->enableAllBuses();
    }

    // Configure output bus
    if (processor->getBusCount(false) > 0) {
        processor->enableAllBuses();
    }
}

void PluginHost::scanForPlugins(const juce::StringArray &searchPaths) {
    // For specific search paths, always scan (don't use cache)
    juce::ScopedLock lock(pluginLock);
    availablePlugins.clear();
    scanPluginsInPaths(searchPaths, availablePlugins);
    pluginCacheValid = true;
}

void PluginHost::scanPluginsInPaths(const juce::StringArray &searchPaths, juce::Array<PluginInfo> &pluginList) {
    // Clear the provided list
    pluginList.clear();

    DBG("=== Starting Plugin Scan ===");
    DBG("Search paths: " + searchPaths.joinIntoString(", "));

    auto supportedFormats = getSupportedFormats();
    
    // Convert search paths to File objects
    juce::Array<juce::File> searchDirectories;
    for (const auto &path : searchPaths) {
        juce::File dir(path);
        if (dir.exists() && dir.isDirectory()) {
            searchDirectories.add(dir);
            DBG("Added search path: " + dir.getFullPathName());
        } else {
            DBG("Invalid search path: " + path);
        }
    }

    // Scan each directory for all supported plugin formats
    for (const auto &searchDir : searchDirectories) {
        if (!searchDir.exists() || !searchDir.isDirectory()) {
            continue;
        }

        DBG("Scanning directory recursively: " + searchDir.getFullPathName());

        // Scan for each supported format (recursive search)
        for (const auto &formatInfo : supportedFormats) {
            // Find files with the format's extensions (recursive)
            for (const auto &extension : formatInfo.fileExtensions) {
                juce::Array<juce::File> pluginFiles;
                searchDir.findChildFiles(pluginFiles, juce::File::findFiles, true, "*." + extension);
                
                for (const auto &pluginFile : pluginFiles) {
                    if (auto *format = getFormatForFile(pluginFile)) {
                        processPluginFile(pluginFile, format, pluginList);
                    }
                }
            }

            // Find directories with the format's extensions (bundles, recursive)
            for (const auto &extension : formatInfo.directoryExtensions) {
                juce::Array<juce::File> pluginBundles;
                searchDir.findChildFiles(pluginBundles, juce::File::findDirectories, true, "*." + extension);
                
                for (const auto &bundleFile : pluginBundles) {
                    if (auto *format = getFormatForFile(bundleFile)) {
                        processPluginBundle(bundleFile, format, pluginList);
                    }
                }
            }
        }
    }

    DBG("Final available plugins count: " + juce::String(pluginList.size()));
    DBG("=== Plugin Scan Complete ===");
}

void PluginHost::addPluginToList(const juce::PluginDescription &description) {
    // Check if plugin is already in the list
    for (const auto &existing : availablePlugins) {
        if (existing.fileOrIdentifier == description.fileOrIdentifier && existing.name == description.name) {
            return; // Already exists
        }
    }

    // Add to list
    availablePlugins.add(createPluginInfo(description));
}

//==============================================================================
// Architecture Detection Methods
bool PluginHost::isHostArchitecture64Bit() const {
#if JUCE_64BIT
    return true;
#else
    return false;
#endif
}

bool PluginHost::isPluginArchitectureCompatible(const juce::File &pluginFile) const {
    bool hostIs64Bit = isHostArchitecture64Bit();
    juce::String pluginArch = getPluginArchitecture(pluginFile);

    if (pluginArch.isEmpty()) {
        // If we can't determine architecture, assume compatible
        return true;
    }

    bool pluginIs64Bit = pluginArch.containsIgnoreCase("64") || pluginArch.containsIgnoreCase("x64");

    // Compatible if both are same architecture
    return hostIs64Bit == pluginIs64Bit;
}

juce::String PluginHost::getPluginArchitecture(const juce::File &pluginFile) const {
    if (!pluginFile.exists()) {
        return "Unknown";
    }

#if JUCE_WINDOWS
    // For Windows, check the file or bundle structure
    if (pluginFile.isDirectory()) {
        // VST3 bundle - check subdirectories
        auto contentsDir = pluginFile.getChildFile("Contents");
        if (contentsDir.exists()) {
            if (contentsDir.getChildFile("x86_64-win").exists()) {
                return "x64";
            } else if (contentsDir.getChildFile("x86-win").exists()) {
                return "x86";
            }
        }
    } else {
        // Single VST3 file - we need to check the PE header
        return analyzeWindowsPEArchitecture(pluginFile);
    }
#elif JUCE_MAC
    // For macOS, check bundle structure
    if (pluginFile.isDirectory()) {
        auto contentsDir = pluginFile.getChildFile("Contents");
        if (contentsDir.exists()) {
            auto macOSDir = contentsDir.getChildFile("MacOS");
            if (macOSDir.exists()) {
                // Check for universal binary or specific architectures
                auto binaries = macOSDir.findChildFiles(juce::File::findFiles, false);
                if (!binaries.isEmpty()) {
                    return analyzeMacBinaryArchitecture(binaries[0]);
                }
            }
        }
    }
#endif

    return "Unknown";
}

juce::String PluginHost::analyzeWindowsPEArchitecture(const juce::File &pluginFile) const {
#if JUCE_WINDOWS
    // Read PE header to determine architecture
    juce::FileInputStream stream(pluginFile);
    if (!stream.openedOk()) {
        return "Unknown";
    }

    // Read DOS header
    char dosHeader[64];
    if (stream.read(dosHeader, 64) != 64) {
        return "Unknown";
    }

    // Check DOS signature
    if (dosHeader[0] != 'M' || dosHeader[1] != 'Z') {
        return "Unknown";
    }

    // Get PE header offset
    uint32_t peOffset = *reinterpret_cast<uint32_t*>(&dosHeader[60]);
    stream.setPosition(peOffset);

    // Read PE signature
    char peSignature[4];
    if (stream.read(peSignature, 4) != 4) {
        return "Unknown";
    }

    if (memcmp(peSignature, "PE\0\0", 4) != 0) {
        return "Unknown";
    }

    // Read COFF header
    uint16_t machine;
    if (stream.read(&machine, 2) != 2) {
        return "Unknown";
    }

    switch (machine) {
        case 0x8664: // IMAGE_FILE_MACHINE_AMD64
            return "x64";
        case 0x014c: // IMAGE_FILE_MACHINE_I386
            return "x86";
        case 0xAA64: // IMAGE_FILE_MACHINE_ARM64
            return "ARM64";
        default:
            return "Unknown (" + juce::String::toHexString(machine) + ")";
    }
#else
    juce::ignoreUnused(pluginFile);
    return "Unknown";
#endif
}

juce::String PluginHost::analyzeMacBinaryArchitecture(const juce::File &binaryFile) const {
#if JUCE_MAC
    // For macOS, we can use the `file` command or read Mach-O headers
    // For simplicity, assume 64-bit on modern macOS
    juce::ignoreUnused(binaryFile);
    return "x64"; // Most modern Mac plugins are 64-bit
#else
    juce::ignoreUnused(binaryFile);
    return "Unknown";
#endif
}

//==============================================================================
// Consolidated Plugin Scanning
void PluginHost::startPluginScan() {
    if (isCurrentlyScanning) {
        DBG("Already scanning plugins, ignoring request");
        return;
    }

    DBG("Starting plugin scan...");
    isCurrentlyScanning = true;

    // Use timer-based scanning on the main thread (JUCE plugin formats require main thread)
    juce::MessageManager::callAsync([this]() {
        // Prepare the scan
        filesToScan.clear();
        currentScanIndex = 0;

        juce::ScopedLock lock(pluginLock);
        availablePlugins.clear();

        // Get search paths
        juce::StringArray searchPaths;
        if (userConfig != nullptr) {
            searchPaths = userConfig->getVSTSearchPaths();
        } else {
            searchPaths = UserConfig::getDefaultVSTSearchPaths();
        }

        DBG("=== Starting Plugin Scan ===");
        DBG("Search paths: " + searchPaths.joinIntoString(", "));

        // Collect all files to scan
        auto supportedFormats = getSupportedFormats();
        for (const auto &path : searchPaths) {
            juce::File dir(path);
            if (dir.exists() && dir.isDirectory()) {
                DBG("Scanning directory recursively: " + dir.getFullPathName());

                // Find plugin files for all supported formats (recursive search)
                for (const auto &formatInfo : supportedFormats) {
                    // Find files with the format's extensions (recursive)
                    for (const auto &extension : formatInfo.fileExtensions) {
                        juce::Array<juce::File> pluginFiles;
                        dir.findChildFiles(pluginFiles, juce::File::findFiles, true, "*." + extension);
                        filesToScan.addArray(pluginFiles);
                    }

                    // Find directories with the format's extensions (bundles, recursive)
                    for (const auto &extension : formatInfo.directoryExtensions) {
                        juce::Array<juce::File> pluginBundles;
                        dir.findChildFiles(pluginBundles, juce::File::findDirectories, true, "*." + extension);
                        filesToScan.addArray(pluginBundles);
                    }
                }
            }
        }

        DBG("Found " + juce::String(filesToScan.size()) + " plugin files/bundles to scan");

        if (filesToScan.isEmpty()) {
            // No files to scan, finish immediately
            pluginCacheValid = true;
            isCurrentlyScanning = false;

            if (onPluginScanComplete) {
                onPluginScanComplete();
            }
            return;
        }

        // Start the timer-based scanning
        scanningTimer.reset(new PluginScanningTimer(*this));
        scanningTimer->startTimer(10); // Scan one plugin every 10ms
    });
}

void PluginHost::refreshPluginCache() {
    pluginCacheValid = false;
    startPluginScan();
}

void PluginHost::scanNextPlugin()
{
    if (currentScanIndex >= filesToScan.size()) {
        // Scanning complete
        scanningTimer.reset();

        juce::ScopedLock lock(pluginLock);
        pluginCacheValid = true;
        isCurrentlyScanning = false;

        DBG("=== Plugin Scan Complete ===");
        DBG("Final available plugins count: " + juce::String(availablePlugins.size()));

        if (onPluginScanComplete) {
            DBG("Calling onPluginScanComplete callback");
            onPluginScanComplete();
        } else {
            DBG("WARNING: No onPluginScanComplete callback registered!");
        }
        return;
    }

    // Process current file - format detection is handled in getFormatForFile

    // Process current file
    const auto& currentFile = filesToScan[currentScanIndex];

    juce::ScopedLock lock(pluginLock);

    if (auto *format = getFormatForFile(currentFile)) {
        if (currentFile.isDirectory()) {
            processPluginBundle(currentFile, format, availablePlugins);
        } else {
            processPluginFile(currentFile, format, availablePlugins);
        }
    }

    currentScanIndex++;
}

//==============================================================================
// Helper Methods for Multi-Format Support
juce::Array<PluginHost::PluginFormatInfo> PluginHost::getSupportedFormats() const {
    juce::Array<PluginFormatInfo> formats;
    
    // VST3 format
    PluginFormatInfo vst3Info;
    vst3Info.formatName = "VST3";
    vst3Info.fileExtensions.add("vst3");
    vst3Info.directoryExtensions.add("vst3");
    formats.add(vst3Info);
    
    // VST2 format (when available)
    PluginFormatInfo vst2Info;
    vst2Info.formatName = "VST";
    vst2Info.fileExtensions.add("dll");
    vst2Info.fileExtensions.add("vst");
    formats.add(vst2Info);
    
#if JUCE_MAC
    // Audio Unit format (macOS only)
    PluginFormatInfo auInfo;
    auInfo.formatName = "AudioUnit";
    auInfo.directoryExtensions.add("component");
    auInfo.directoryExtensions.add("appex");
    formats.add(auInfo);
#endif

    // CLAP format (future support)
    PluginFormatInfo clapInfo;
    clapInfo.formatName = "CLAP";
    clapInfo.fileExtensions.add("clap");
    formats.add(clapInfo);
    
    return formats;
}

juce::AudioPluginFormat* PluginHost::getFormatForFile(const juce::File &pluginFile) const {
    juce::String extension = pluginFile.getFileExtension().toLowerCase().substring(1);
    
    // Check each registered format
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto *format = formatManager.getFormat(i);
        if (format == nullptr) continue;
        
        juce::String formatName = format->getName().toLowerCase();
        
        // VST3 format
        if (formatName.contains("vst3") && 
            (extension == "vst3" || (pluginFile.isDirectory() && extension == "vst3"))) {
            return format;
        }
        
        // VST2 format
        if (formatName.contains("vst") && !formatName.contains("vst3") &&
            (extension == "dll" || extension == "vst")) {
            return format;
        }
        
#if JUCE_MAC
        // Audio Unit format
        if (formatName.contains("audiounit") &&
            (extension == "component" || extension == "appex")) {
            return format;
        }
#endif
        
        // CLAP format (future)
        if (formatName.contains("clap") && extension == "clap") {
            return format;
        }
    }
    
    return nullptr;
}
