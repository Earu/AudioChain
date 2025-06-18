#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class UserConfig {
  public:
    UserConfig();
    ~UserConfig();

    // VST Search Paths
    void addVSTSearchPath(const juce::String &path);
    void removeVSTSearchPath(const juce::String &path);
    void clearVSTSearchPaths();
    juce::StringArray getVSTSearchPaths() const;
    void setVSTSearchPaths(const juce::StringArray &paths);

    // Configuration persistence
    void saveToFile();
    void loadFromFile();

    // Get default VST paths for the platform
    static juce::StringArray getDefaultVSTSearchPaths();

  private:
    juce::StringArray vstSearchPaths;
    juce::File configFile;

    void initializeDefaults();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UserConfig)
};