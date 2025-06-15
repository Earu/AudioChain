#include "VST3PluginHost.h"
#include "UserConfig.h"

//==============================================================================
VST3PluginHost::VST3PluginHost()
{
    // Initialize format manager with specific formats (avoid VST2)
    formatManager.addFormat(new juce::VST3PluginFormat());
    #if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
    #endif
    
    // Debug: Check what formats are available
    DBG("Format manager initialized with " + juce::String(formatManager.getNumFormats()) + " formats:");
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr)
        {
            DBG("  Format " + juce::String(i) + ": " + format->getName());
        }
    }
    
    // Don't scan on initialization to prevent crashes
    // scanForPlugins(); // Call this manually when needed
}

VST3PluginHost::~VST3PluginHost()
{
    clearAllPlugins();
}

//==============================================================================
void VST3PluginHost::prepareToPlay(int samplesPerBlock, double sampleRate)
{
    juce::ScopedLock lock(pluginLock);
    
    currentBlockSize = samplesPerBlock;
    currentSampleRate = sampleRate;
    
    // Prepare all plugins in the chain
    for (auto* plugin : pluginChain)
    {
        if (plugin->isValid())
        {
            plugin->processor->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
    
    isPrepared = true;
}

void VST3PluginHost::processAudio(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock lock(pluginLock);
    
    if (!isPrepared)
        return;
    
    // Process through each plugin in the chain
    for (auto* plugin : pluginChain)
    {
        if (plugin->isValid() && !plugin->bypassed)
        {
            try
            {
                // Create MIDI buffer (empty for now)
                juce::MidiBuffer midiBuffer;
                
                // Process the audio
                plugin->processor->processBlock(buffer, midiBuffer);
            }
            catch (const std::exception& e)
            {
                // Handle plugin processing errors
                plugin->errorMessage = "Processing error: " + juce::String(e.what());
                plugin->bypassed = true; // Bypass plugin on error
                
                if (onPluginError)
                {
                    onPluginError(pluginChain.indexOf(plugin), plugin->errorMessage);
                }
            }
        }
    }
}

void VST3PluginHost::releaseResources()
{
    juce::ScopedLock lock(pluginLock);
    
    for (auto* plugin : pluginChain)
    {
        if (plugin->isValid())
        {
            plugin->processor->releaseResources();
        }
    }
    
    isPrepared = false;
}

//==============================================================================
bool VST3PluginHost::loadPlugin(const juce::String& pluginPath)
{
    // Find plugin info by path
    for (const auto& pluginInfo : availablePlugins)
    {
        if (pluginInfo.fileOrIdentifier == pluginPath)
        {
            return loadPlugin(pluginInfo);
        }
    }
    return false;
}

bool VST3PluginHost::loadPlugin(const PluginInfo& pluginInfo)
{
    juce::ScopedLock lock(pluginLock);
    
    // Use the stored JUCE description if available, otherwise create one manually
    juce::PluginDescription description;
    if (pluginInfo.hasJuceDescription)
    {
        description = pluginInfo.juceDescription;
        DBG("Using stored JUCE description for: " + pluginInfo.name);
    }
    else
    {
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
        formatManager.createPluginInstance(description, currentSampleRate, currentBlockSize, errorMessage)
    );
    
    if (!processor)
    {
        DBG("Failed to create plugin instance for: " + pluginInfo.name);
        DBG("Error message: " + errorMessage);
        DBG("Plugin path: " + pluginInfo.fileOrIdentifier);
        DBG("Plugin format: " + pluginInfo.pluginFormatName);
        
        if (onPluginError)
        {
            onPluginError(-1, "Failed to load plugin: " + errorMessage);
        }
        return false;
    }
    
    // Validate plugin
    if (!validatePlugin(processor.get()))
    {
        if (onPluginError)
        {
            onPluginError(-1, "Plugin validation failed");
        }
        return false;
    }
    
    // Create plugin instance wrapper
    auto* instance = new PluginInstance();
    instance->processor = std::move(processor);
    instance->info = pluginInfo;
    instance->bypassed = false;
    
    // Initialize the plugin
    initializePlugin(instance);
    
    // Add to chain
    pluginChain.add(instance);
    
    // Prepare plugin if we're already prepared
    if (isPrepared)
    {
        instance->processor->prepareToPlay(currentSampleRate, currentBlockSize);
    }
    
    // Notify listeners
    if (onPluginChainChanged)
    {
        onPluginChainChanged();
    }
    
    return true;
}

void VST3PluginHost::unloadPlugin(int index)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        // Close editor if open
        closeEditorForPlugin(index);
        
        // Remove from chain
        pluginChain.remove(index);
        
        // Notify listeners
        if (onPluginChainChanged)
        {
            onPluginChainChanged();
        }
    }
}

void VST3PluginHost::clearAllPlugins()
{
    juce::ScopedLock lock(pluginLock);
    
    // Close all editors
    for (int i = 0; i < pluginChain.size(); ++i)
    {
        closeEditorForPlugin(i);
    }
    
    // Clear chain
    pluginChain.clear();
    
    // Notify listeners
    if (onPluginChainChanged)
    {
        onPluginChainChanged();
    }
}

//==============================================================================
void VST3PluginHost::movePlugin(int fromIndex, int toIndex)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(fromIndex, pluginChain.size()) &&
        juce::isPositiveAndBelow(toIndex, pluginChain.size()) &&
        fromIndex != toIndex)
    {
        pluginChain.move(fromIndex, toIndex);
        
        if (onPluginChainChanged)
        {
            onPluginChainChanged();
        }
    }
}

void VST3PluginHost::bypassPlugin(int index, bool shouldBypass)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        pluginChain[index]->bypassed = shouldBypass;
    }
}

bool VST3PluginHost::isPluginBypassed(int index) const
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        return pluginChain[index]->bypassed;
    }
    return false;
}

//==============================================================================
juce::AudioProcessor* VST3PluginHost::getPlugin(int index)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

const juce::AudioProcessor* VST3PluginHost::getPlugin(int index) const
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        return pluginChain[index]->processor.get();
    }
    return nullptr;
}

VST3PluginHost::PluginInfo VST3PluginHost::getPluginInfo(int index) const
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        return pluginChain[index]->info;
    }
    return {};
}

//==============================================================================
void VST3PluginHost::scanForPlugins()
{
    availablePlugins.clear();
    scanVST3Plugins();
}

juce::AudioProcessorEditor* VST3PluginHost::createEditorForPlugin(int index)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        auto* instance = pluginChain[index];
        if (instance->isValid() && !instance->editor)
        {
            instance->editor.reset(instance->processor->createEditor());
            return instance->editor.get();
        }
    }
    return nullptr;
}

void VST3PluginHost::closeEditorForPlugin(int index)
{
    juce::ScopedLock lock(pluginLock);
    
    if (juce::isPositiveAndBelow(index, pluginChain.size()))
    {
        pluginChain[index]->editor.reset();
    }
}

//==============================================================================
juce::ValueTree VST3PluginHost::getState() const
{
    juce::ScopedLock lock(pluginLock);
    
    juce::ValueTree state("PluginChain");
    
    for (int i = 0; i < pluginChain.size(); ++i)
    {
        auto* instance = pluginChain[i];
        if (instance->isValid())
        {
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

void VST3PluginHost::setState(const juce::ValueTree& state)
{
    if (!state.hasType("PluginChain"))
        return;
    
    clearAllPlugins();
    
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        juce::ValueTree pluginState = state.getChild(i);
        if (pluginState.hasType("Plugin"))
        {
            PluginInfo info;
            info.name = pluginState.getProperty("name", "");
            info.manufacturer = pluginState.getProperty("manufacturer", "");
            info.version = pluginState.getProperty("version", "");
            info.fileOrIdentifier = pluginState.getProperty("fileOrIdentifier", "");
            
            if (loadPlugin(info))
            {
                int pluginIndex = pluginChain.size() - 1;
                
                // Restore bypass state
                bool bypassed = pluginState.getProperty("bypassed", false);
                bypassPlugin(pluginIndex, bypassed);
                
                // Restore plugin internal state
                juce::String stateString = pluginState.getProperty("state", "");
                if (stateString.isNotEmpty())
                {
                    juce::MemoryBlock stateBlock;
                    stateBlock.fromBase64Encoding(stateString);
                    
                    if (auto* processor = getPlugin(pluginIndex))
                    {
                        processor->setStateInformation(stateBlock.getData(), (int)stateBlock.getSize());
                    }
                }
            }
        }
    }
}

//==============================================================================
VST3PluginHost::PluginInfo VST3PluginHost::createPluginInfo(const juce::PluginDescription& description)
{
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
    
    return info;
}

bool VST3PluginHost::validatePlugin(juce::AudioProcessor* processor)
{
    if (!processor)
        return false;
    
    // Basic validation checks
    if (processor->getName().isEmpty())
        return false;
    
    // Check if plugin accepts stereo input/output
    if (processor->getMainBusNumInputChannels() > 2 || processor->getMainBusNumOutputChannels() > 2)
    {
        // For now, we only support mono/stereo plugins
        return false;
    }
    
    return true;
}

void VST3PluginHost::initializePlugin(PluginInstance* instance)
{
    if (!instance || !instance->isValid())
        return;
    
    // Set up bus layouts for stereo processing
    auto* processor = instance->processor.get();
    
    // Configure input bus
    if (processor->getBusCount(true) > 0)
    {
        processor->enableAllBuses();
    }
    
    // Configure output bus
    if (processor->getBusCount(false) > 0)
    {
        processor->enableAllBuses();
    }
}

void VST3PluginHost::scanVST3Plugins()
{
    // Use configured search paths if available, otherwise use defaults
    juce::StringArray searchPaths;
    if (userConfig != nullptr)
    {
        searchPaths = userConfig->getVSTSearchPaths();
    }
    else
    {
        searchPaths = UserConfig::getDefaultVSTSearchPaths();
    }
    
    scanForPlugins(searchPaths);
}

void VST3PluginHost::scanForPlugins(const juce::StringArray& searchPaths)
{
    // Clear existing plugins
    availablePlugins.clear();

    DBG("=== Starting VST3 Plugin Scan ===");
    DBG("Search paths: " + searchPaths.joinIntoString(", "));

    // Check if we have VST3 format available
    juce::AudioPluginFormat* vst3Format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr && format->getName().containsIgnoreCase("VST3"))
        {
            vst3Format = format;
            DBG("Found VST3 format: " + format->getName());
            break;
        }
    }

    if (vst3Format != nullptr)
    {
        // Convert search paths to File objects
        juce::Array<juce::File> vstDirectories;
        for (const auto& path : searchPaths)
        {
            juce::File dir(path);
            if (dir.exists() && dir.isDirectory())
            {
                vstDirectories.add(dir);
                DBG("Added search path: " + dir.getFullPathName());
            }
            else
            {
                DBG("Invalid search path: " + path);
            }
        }
        
        for (const auto& vstDir : vstDirectories)
        {
            if (vstDir.exists() && vstDir.isDirectory())
            {
                // VST3 plugins are bundles (directories), not files
                juce::Array<juce::File> vstBundles;
                vstDir.findChildFiles(vstBundles, juce::File::findDirectories, false, "*.vst3");
                DBG("Directory: " + vstDir.getFullPathName() + " contains " + juce::String(vstBundles.size()) + " .vst3 bundles");
            
            // Add the first few plugins manually to test
            int addedCount = 0;
            for (const auto& vstBundle : vstBundles)
            {
                if (addedCount >= 3) break; // Limit to 3 plugins for testing
                
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
                
                if (validBundle)
        {
                    DBG("    Valid VST3 bundle structure");
                    
                    // Try to get proper plugin description using JUCE
                    juce::OwnedArray<juce::PluginDescription> descriptions;
                    vst3Format->findAllTypesForFile(descriptions, vstBundle.getFullPathName());
                    
                    bool foundDescription = descriptions.size() > 0;
                    if (foundDescription)
                    {
                        DBG("    Successfully read plugin description");
                    }
                    else
                    {
                        DBG("    Could not read plugin description, using fallback");
                    }
                    
                    // Create plugin info from description or fallback
                    PluginInfo info;
                    if (foundDescription)
    {
                        auto* description = descriptions[0]; // Use first description
                        info.name = description->name.isNotEmpty() ? description->name : vstBundle.getFileNameWithoutExtension();
                        info.manufacturer = description->manufacturerName.isNotEmpty() ? description->manufacturerName : "Unknown";
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
                    }
                    else
                    {
                        // Fallback to basic info
                        info.name = vstBundle.getFileNameWithoutExtension();
                        info.manufacturer = "Unknown";
                        info.version = "1.0";
                        info.pluginFormatName = "VST3";
                        info.fileOrIdentifier = vstBundle.getFullPathName();
                        info.numInputChannels = 2;
                        info.numOutputChannels = 2;
                        info.isInstrument = false;
                        info.hasEditor = true;
                        info.hasJuceDescription = false;
                    }
                    
                    availablePlugins.add(info);
                    addedCount++;
                    
                    DBG("    Added plugin: " + info.name + " by " + info.manufacturer);
            }
            else
            {
                    DBG("    Invalid VST3 bundle structure for platform");
                }
                            }
            }
            else
            {
                DBG("VST3 directory doesn't exist: " + vstDir.getFullPathName());
            }
        }
        }
    else
    {
        DBG("ERROR: VST3 format not found in format manager!");
    }
    
    DBG("Final available plugins count: " + juce::String(availablePlugins.size()));
    DBG("=== VST3 Plugin Scan Complete ===");
}

void VST3PluginHost::addPluginToList(const juce::PluginDescription& description)
{
    // Check if plugin is already in the list
    for (const auto& existing : availablePlugins)
    {
        if (existing.fileOrIdentifier == description.fileOrIdentifier &&
            existing.name == description.name)
        {
            return; // Already exists
        }
    }
    
    // Add to list
    availablePlugins.add(createPluginInfo(description));
} 
 