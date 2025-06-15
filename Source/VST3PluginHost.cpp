#include "VST3PluginHost.h"

//==============================================================================
VST3PluginHost::VST3PluginHost()
{
    // Initialize format manager with VST3 support
    formatManager.addDefaultFormats();
    
    // The default formats include VST3, AU, VST2, etc.
    // No need to manually add VST format as it's included in addDefaultFormats()
    
    // Scan for plugins on initialization
    scanForPlugins();
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
    
    // Create plugin description
    juce::PluginDescription description;
    description.name = pluginInfo.name;
    description.manufacturerName = pluginInfo.manufacturer;
    description.version = pluginInfo.version;
    description.pluginFormatName = pluginInfo.pluginFormatName;
    description.fileOrIdentifier = pluginInfo.fileOrIdentifier;
    description.numInputChannels = pluginInfo.numInputChannels;
    description.numOutputChannels = pluginInfo.numOutputChannels;
    description.isInstrument = pluginInfo.isInstrument;
    description.hasSharedContainer = false;
    
    // Create plugin instance
    juce::String errorMessage;
    std::unique_ptr<juce::AudioProcessor> processor(
        formatManager.createPluginInstance(description, currentSampleRate, currentBlockSize, errorMessage)
    );
    
    if (!processor)
    {
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
    // Clear existing plugins
    availablePlugins.clear();
    
    // Create a known plugin list for discovered plugins
    juce::KnownPluginList pluginList;
    
    // Debug: Print available formats
    DBG("Number of formats available: " + juce::String(formatManager.getNumFormats()));
    
    // Get all available plugin formats from the format manager
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr)
        {
            DBG("Scanning format: " + format->getName());
            
            // Get default search paths for this format
            juce::FileSearchPath searchPaths = format->getDefaultLocationsToSearch();
            
            // Debug: Print search paths
            for (int pathIdx = 0; pathIdx < searchPaths.getNumPaths(); ++pathIdx)
            {
                DBG("Search path: " + searchPaths[pathIdx].getFullPathName());
            }
            
            // Scan for plugins of this format
            juce::PluginDirectoryScanner scanner(pluginList, *format, searchPaths, true, juce::File());
            
            juce::String pluginBeingScanned;
            while (scanner.scanNextFile(false, pluginBeingScanned))
            {
                DBG("Scanning: " + pluginBeingScanned);
            }
        }
    }
    
    DBG("Total plugins found: " + juce::String(pluginList.getNumTypes()));
    
    // Add all found plugins to our available plugins list
    for (int j = 0; j < pluginList.getNumTypes(); ++j)
    {
        auto* description = pluginList.getType(j);
        if (description != nullptr)
        {
            DBG("Adding plugin: " + description->name + " (" + description->pluginFormatName + ")");
            addPluginToList(*description);
        }
    }
    
    DBG("Available plugins in list: " + juce::String(availablePlugins.size()));
    
    // If no plugins found, manually check common VST3 locations
    if (availablePlugins.size() == 0)
    {
        DBG("No plugins found with automatic scanning, checking manual locations...");
        
        juce::Array<juce::File> manualLocations;
        manualLocations.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
        manualLocations.add(juce::File(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Library/Audio/Plug-Ins/VST3")));
        manualLocations.add(juce::File("/System/Library/Audio/Plug-Ins/VST3"));
        
        for (auto& location : manualLocations)
        {
            if (location.exists())
            {
                DBG("Checking manual location: " + location.getFullPathName());
                juce::Array<juce::File> vstFiles;
                location.findChildFiles(vstFiles, juce::File::findFiles, true, "*.vst3");
                DBG("Found " + juce::String(vstFiles.size()) + " VST3 files in " + location.getFullPathName());
            }
            else
            {
                DBG("Manual location doesn't exist: " + location.getFullPathName());
            }
        }
    }
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
 