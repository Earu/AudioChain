#include "UserConfig.h"

UserConfig::UserConfig() {
    // Get application data directory for storing config
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto audioChainDir = appDataDir.getChildFile("AudioChain");

    if (!audioChainDir.exists())
        audioChainDir.createDirectory();

    configFile = audioChainDir.getChildFile("config.xml");

    initializeDefaults();
    loadFromFile();
}

UserConfig::~UserConfig() { saveToFile(); }

void UserConfig::addVSTSearchPath(const juce::String &path) {
    if (path.isNotEmpty() && !vstSearchPaths.contains(path)) {
        vstSearchPaths.add(path);
        saveToFile();
    }
}

void UserConfig::removeVSTSearchPath(const juce::String &path) {
    vstSearchPaths.removeString(path);
    saveToFile();
}

void UserConfig::clearVSTSearchPaths() {
    vstSearchPaths.clear();
    saveToFile();
}

juce::StringArray UserConfig::getVSTSearchPaths() const { return vstSearchPaths; }

void UserConfig::setVSTSearchPaths(const juce::StringArray &paths) {
    vstSearchPaths = paths;
    saveToFile();
}

void UserConfig::saveToFile() {
    juce::XmlElement config("AudioChainConfig");

    auto vstPathsElement = config.createNewChildElement("VSTSearchPaths");
    for (const auto &path : vstSearchPaths) {
        auto pathElement = vstPathsElement->createNewChildElement("Path");
        pathElement->addTextElement(path);
    }

    config.writeTo(configFile);
}

void UserConfig::loadFromFile() {
    if (!configFile.exists())
        return;

    auto config = juce::XmlDocument::parse(configFile);
    if (config == nullptr)
        return;

    auto vstPathsElement = config->getChildByName("VSTSearchPaths");
    if (vstPathsElement != nullptr) {
        vstSearchPaths.clear();
        for (auto *pathElement : vstPathsElement->getChildIterator()) {
            if (pathElement->hasTagName("Path")) {
                auto path = pathElement->getAllSubText();
                if (path.isNotEmpty())
                    vstSearchPaths.add(path);
            }
        }
    }
}

juce::StringArray UserConfig::getDefaultVSTSearchPaths() {
    juce::StringArray defaultPaths;

#if JUCE_MAC
    defaultPaths.add("/Library/Audio/Plug-Ins/VST3");
    defaultPaths.add("~/Library/Audio/Plug-Ins/VST3");
#elif JUCE_WINDOWS
    defaultPaths.add("C:\\Program Files\\Common Files\\VST3");
    defaultPaths.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("VST3")
                         .getFullPathName());
#elif JUCE_LINUX
    defaultPaths.add("~/.vst3");
    defaultPaths.add("/usr/lib/vst3");
    defaultPaths.add("/usr/local/lib/vst3");
#endif

    return defaultPaths;
}

void UserConfig::initializeDefaults() {
    if (vstSearchPaths.isEmpty()) {
        vstSearchPaths = getDefaultVSTSearchPaths();
    }
}