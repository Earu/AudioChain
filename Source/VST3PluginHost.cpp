#include "VST3PluginHost.h"
#include "UserConfig.h"
#include <vector>
#include <cmath>

//==============================================================================
VST3PluginHost::VST3PluginHost() {
    // Initialize format manager with specific formats (avoid VST2)
    formatManager.addFormat(new juce::VST3PluginFormat());
#if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
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

VST3PluginHost::~VST3PluginHost() { clearAllPlugins(); }

//==============================================================================
void VST3PluginHost::prepareToPlay(int samplesPerBlock, double sampleRate) {
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

void VST3PluginHost::processAudio(juce::AudioBuffer<float> &buffer) {
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

void VST3PluginHost::releaseResources() {
    juce::ScopedLock lock(pluginLock);

    for (auto *plugin : pluginChain) {
        if (plugin->isValid()) {
            plugin->processor->releaseResources();
        }
    }

    isPrepared = false;
}

//==============================================================================
bool VST3PluginHost::loadPlugin(const juce::String &pluginPath) {
    // Find plugin info by path
    for (const auto &pluginInfo : availablePlugins) {
        if (pluginInfo.fileOrIdentifier == pluginPath) {
            return loadPlugin(pluginInfo);
        }
    }
    return false;
}

bool VST3PluginHost::loadPlugin(const PluginInfo &pluginInfo) {
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

void VST3PluginHost::unloadPlugin(int index) {
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

void VST3PluginHost::clearAllPlugins() {
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
void VST3PluginHost::movePlugin(int fromIndex, int toIndex) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(fromIndex, pluginChain.size()) &&
        juce::isPositiveAndBelow(toIndex, pluginChain.size()) && fromIndex != toIndex) {
        pluginChain.move(fromIndex, toIndex);

        if (onPluginChainChanged) {
            onPluginChainChanged();
        }
    }
}

void VST3PluginHost::bypassPlugin(int index, bool shouldBypass) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        pluginChain[index]->bypassed = shouldBypass;
    }
}

bool VST3PluginHost::isPluginBypassed(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->bypassed;
    }
    return false;
}

//==============================================================================
juce::AudioProcessor *VST3PluginHost::getPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

const juce::AudioProcessor *VST3PluginHost::getPlugin(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

VST3PluginHost::PluginInfo VST3PluginHost::getPluginInfo(int index) const {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        return pluginChain[index]->info;
    }
    return {};
}

//==============================================================================
void VST3PluginHost::scanForPlugins() {
    // Check if we already have cached plugins
    if (pluginCacheValid && !availablePlugins.isEmpty()) {
        DBG("Using cached plugin list (" + juce::String(availablePlugins.size()) + " plugins)");
        return;
    }

    // Cache is invalid or empty, perform async scan
    scanForPluginsAsync();
}

juce::AudioProcessorEditor *VST3PluginHost::createEditorForPlugin(int index) {
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

void VST3PluginHost::closeEditorForPlugin(int index) {
    juce::ScopedLock lock(pluginLock);

    if (juce::isPositiveAndBelow(index, pluginChain.size())) {
        pluginChain[index]->editor.reset();
    }
}

//==============================================================================
juce::ValueTree VST3PluginHost::getState() const {
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

void VST3PluginHost::setState(const juce::ValueTree &state) {
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

void VST3PluginHost::processVST3File(const juce::File &vstFile, juce::AudioPluginFormat *vst3Format, juce::Array<PluginInfo> &pluginList) {
    DBG("  Found VST3 file: " + vstFile.getFullPathName());

    // Check architecture compatibility FIRST, before JUCE tries to load it
    juce::String architecture = getPluginArchitecture(vstFile);
    bool isCompatible = isPluginArchitectureCompatible(vstFile);

    DBG("    Plugin architecture: " + architecture + ", Compatible: " + (isCompatible ? "Yes" : "No"));

    // Skip incompatible plugins entirely
    if (!isCompatible) {
        DBG("    Skipped incompatible plugin: " + vstFile.getFileNameWithoutExtension() +
            " (" + architecture + " vs host " + (isHostArchitecture64Bit() ? "x64" : "x86") + ")");
        return;
    }

    // Try to get proper plugin description using JUCE (only for compatible plugins)
    juce::OwnedArray<juce::PluginDescription> descriptions;
    vst3Format->findAllTypesForFile(descriptions, vstFile.getFullPathName());

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
                                                   : vstFile.getFileNameWithoutExtension();
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
        info.name = vstFile.getFileNameWithoutExtension();
        info.manufacturer = "Unknown";
        info.version = "1.0";
        info.pluginFormatName = "VST3";
        info.fileOrIdentifier = vstFile.getFullPathName();
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

void VST3PluginHost::processVST3Bundle(const juce::File &vstBundle, juce::AudioPluginFormat *vst3Format, juce::Array<PluginInfo> &pluginList) {
    DBG("  Found VST3 bundle: " + vstBundle.getFullPathName());

    // Check if it's a valid VST3 bundle by looking for platform-specific structure
    auto contentsDir = vstBundle.getChildFile("Contents");
    bool validBundle = false;

#if JUCE_MAC
    auto macOSDir = contentsDir.getChildFile("MacOS");
    validBundle = contentsDir.exists() && macOSDir.exists();
#elif JUCE_WINDOWS
    auto x64Dir = contentsDir.getChildFile("x86_64-win");
    validBundle = contentsDir.exists() && x64Dir.exists();
#endif

    if (!validBundle) {
        DBG("    Invalid VST3 bundle structure for platform");
        return;
    }

    DBG("    Valid VST3 bundle structure");

    // Check architecture compatibility FIRST, before JUCE tries to load it
    juce::String architecture = getPluginArchitecture(vstBundle);
    bool isCompatible = isPluginArchitectureCompatible(vstBundle);

    DBG("    Plugin architecture: " + architecture + ", Compatible: " + (isCompatible ? "Yes" : "No"));

    // Skip incompatible plugins entirely
    if (!isCompatible) {
        DBG("    Skipped incompatible plugin: " + vstBundle.getFileNameWithoutExtension() +
            " (" + architecture + " vs host " + (isHostArchitecture64Bit() ? "x64" : "x86") + ")");
        return;
    }

    // Try to get proper plugin description using JUCE (only for compatible plugins)
    juce::OwnedArray<juce::PluginDescription> descriptions;
    vst3Format->findAllTypesForFile(descriptions, vstBundle.getFullPathName());

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
                                                   : vstBundle.getFileNameWithoutExtension();
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
        info.name = vstBundle.getFileNameWithoutExtension();
        info.manufacturer = "Unknown";
        info.version = "1.0";
        info.pluginFormatName = "VST3";
        info.fileOrIdentifier = vstBundle.getFullPathName();
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
VST3PluginHost::PluginInfo VST3PluginHost::createPluginInfo(const juce::PluginDescription &description) {
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

bool VST3PluginHost::validatePlugin(juce::AudioProcessor *processor) {
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

void VST3PluginHost::initializePlugin(PluginInstance *instance) {
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

void VST3PluginHost::scanForPlugins(const juce::StringArray &searchPaths) {
    // For specific search paths, always scan (don't use cache)
    juce::ScopedLock lock(pluginLock);
    availablePlugins.clear();
    scanPluginsInPaths(searchPaths, availablePlugins);
    pluginCacheValid = true;
}

void VST3PluginHost::scanForPluginsSync() {
    // Force synchronous scan, bypassing cache
    juce::ScopedLock lock(pluginLock);

    // Use configured search paths if available, otherwise use defaults
    juce::StringArray searchPaths;
    if (userConfig != nullptr) {
        searchPaths = userConfig->getVSTSearchPaths();
    } else {
        searchPaths = UserConfig::getDefaultVSTSearchPaths();
    }

    availablePlugins.clear();
    scanPluginsInPaths(searchPaths, availablePlugins);
    pluginCacheValid = true;
    isCurrentlyScanning = false;
}

void VST3PluginHost::scanPluginsInPaths(const juce::StringArray &searchPaths, juce::Array<PluginInfo> &pluginList) {
    // Clear the provided list
    pluginList.clear();

    DBG("=== Starting VST3 Plugin Scan ===");
    DBG("Search paths: " + searchPaths.joinIntoString(", "));

    // Check if we have VST3 format available
    juce::AudioPluginFormat *vst3Format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto *format = formatManager.getFormat(i);
        if (format != nullptr && format->getName().containsIgnoreCase("VST3")) {
            vst3Format = format;
            DBG("Found VST3 format: " + format->getName());
            break;
        }
    }

    if (vst3Format != nullptr) {
        // Convert search paths to File objects
        juce::Array<juce::File> vstDirectories;
        for (const auto &path : searchPaths) {
            juce::File dir(path);
            if (dir.exists() && dir.isDirectory()) {
                vstDirectories.add(dir);
                DBG("Added search path: " + dir.getFullPathName());
            } else {
                DBG("Invalid search path: " + path);
            }
        }

        for (const auto &vstDir : vstDirectories) {
            if (vstDir.exists() && vstDir.isDirectory()) {
                // Scan for both VST3 files and bundles
                juce::Array<juce::File> vstFiles;
                juce::Array<juce::File> vstBundles;

                // Find VST3 files (single files, common on Windows)
                vstDir.findChildFiles(vstFiles, juce::File::findFiles, false, "*.vst3");
                // Find VST3 bundles (directories, common on macOS)
                vstDir.findChildFiles(vstBundles, juce::File::findDirectories, false, "*.vst3");

                DBG("Directory: " + vstDir.getFullPathName() + " contains " +
                    juce::String(vstFiles.size()) + " .vst3 files and " +
                    juce::String(vstBundles.size()) + " .vst3 bundles");

                // Process single VST3 files first
                for (const auto &vstFile : vstFiles) {
                    processVST3File(vstFile, vst3Format, pluginList);
                }

                // Process VST3 bundles (mainly for macOS)
                for (const auto &vstBundle : vstBundles) {
                    processVST3Bundle(vstBundle, vst3Format, pluginList);
                }
            } else {
                DBG("VST3 directory doesn't exist: " + vstDir.getFullPathName());
            }
        }
    } else {
        DBG("ERROR: VST3 format not found in format manager!");
    }

    DBG("Final available plugins count: " + juce::String(pluginList.size()));
    DBG("=== VST3 Plugin Scan Complete ===");
}

void VST3PluginHost::addPluginToList(const juce::PluginDescription &description) {
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
bool VST3PluginHost::isHostArchitecture64Bit() const {
#if JUCE_64BIT
    return true;
#else
    return false;
#endif
}

bool VST3PluginHost::isPluginArchitectureCompatible(const juce::File &pluginFile) const {
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

juce::String VST3PluginHost::getPluginArchitecture(const juce::File &pluginFile) const {
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

juce::String VST3PluginHost::analyzeWindowsPEArchitecture(const juce::File &pluginFile) const {
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

juce::String VST3PluginHost::analyzeMacBinaryArchitecture(const juce::File &binaryFile) const {
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
// Plugin Scanning Thread
class VST3PluginHost::PluginScanningThread : public juce::Thread
{
public:
    PluginScanningThread(VST3PluginHost& host)
        : juce::Thread("VST3PluginScanning"), pluginHost(host) {}

    void run() override
    {
        pluginHost.performPluginScan();
    }

private:
    VST3PluginHost& pluginHost;
};

//==============================================================================
// Async Scanning Methods
void VST3PluginHost::scanForPluginsAsync()
{
    if (isCurrentlyScanning) {
        DBG("Already scanning plugins, ignoring request");
        return; // Already scanning
    }

    DBG("Starting async plugin scan...");
    isCurrentlyScanning = true;

    // Create and start scanning thread
    scanningThread = std::make_unique<PluginScanningThread>(*this);
    scanningThread->startThread();
}

void VST3PluginHost::refreshPluginCache()
{
    pluginCacheValid = false;
    scanForPluginsAsync();
}

void VST3PluginHost::performPluginScan()
{
    // JUCE plugin formats must be used on the main thread only!
    // So we'll use a timer to do non-blocking scanning on the main thread
    juce::MessageManager::callAsync([this]() {
        startMainThreadScan();
    });
}

//==============================================================================
// Main Thread Scanning Timer
class VST3PluginHost::PluginScanningTimer : public juce::Timer
{
public:
    PluginScanningTimer(VST3PluginHost& host) : pluginHost(host) {}

    void timerCallback() override
    {
        pluginHost.scanNextPlugin();
    }

private:
    VST3PluginHost& pluginHost;
};

void VST3PluginHost::startMainThreadScan()
{
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

    DBG("=== Starting Non-Blocking VST3 Plugin Scan ===");
    DBG("Search paths: " + searchPaths.joinIntoString(", "));

    // Collect all files to scan
    for (const auto &path : searchPaths) {
        juce::File dir(path);
        if (dir.exists() && dir.isDirectory()) {
            DBG("Scanning directory: " + dir.getFullPathName());

            // Find VST3 files and bundles
            juce::Array<juce::File> vstFiles;
            juce::Array<juce::File> vstBundles;
            dir.findChildFiles(vstFiles, juce::File::findFiles, false, "*.vst3");
            dir.findChildFiles(vstBundles, juce::File::findDirectories, false, "*.vst3");

            filesToScan.addArray(vstFiles);
            filesToScan.addArray(vstBundles);
        }
    }

    DBG("Found " + juce::String(filesToScan.size()) + " VST3 files/bundles to scan");

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
    scanningTimer = std::make_unique<PluginScanningTimer>(*this);
    scanningTimer->startTimer(10); // Scan one plugin every 10ms
}

void VST3PluginHost::scanNextPlugin()
{
    if (currentScanIndex >= filesToScan.size()) {
        // Scanning complete
        scanningTimer.reset();

        juce::ScopedLock lock(pluginLock);
        pluginCacheValid = true;
        isCurrentlyScanning = false;

        DBG("=== VST3 Plugin Scan Complete ===");
        DBG("Final available plugins count: " + juce::String(availablePlugins.size()));

        if (onPluginScanComplete) {
            DBG("Calling onPluginScanComplete callback");
            onPluginScanComplete();
        } else {
            DBG("WARNING: No onPluginScanComplete callback registered!");
        }
        return;
    }

    // Get VST3 format
    juce::AudioPluginFormat *vst3Format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto *format = formatManager.getFormat(i);
        if (format != nullptr && format->getName().containsIgnoreCase("VST3")) {
            vst3Format = format;
            break;
        }
    }

    if (vst3Format == nullptr) {
        DBG("ERROR: VST3 format not found!");
        currentScanIndex = filesToScan.size(); // Skip to end
        return;
    }

    // Process current file
    const auto& currentFile = filesToScan[currentScanIndex];

    juce::ScopedLock lock(pluginLock);

    if (currentFile.isDirectory()) {
        processVST3Bundle(currentFile, vst3Format, availablePlugins);
    } else {
        processVST3File(currentFile, vst3Format, availablePlugins);
    }

    currentScanIndex++;
}
