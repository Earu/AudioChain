cmake_minimum_required(VERSION 3.15)

project(AudioChain VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download and setup JUCE using FetchContent (modern approach)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)
FetchContent_MakeAvailable(JUCE)

# Create the application target
juce_add_gui_app(AudioChain
    PRODUCT_NAME "AudioChain"
    VERSION 1.0.0
    COMPANY_NAME "AudioChain"
    BUNDLE_ID "com.audiochain.AudioChain"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "AudioChain needs microphone access to process audio input through VST plugins in real-time."
)

# Add source files
target_sources(AudioChain PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/PluginHost.cpp
    Source/PluginHost.h
    Source/AudioProcessor.cpp
    Source/AudioProcessor.h
    Source/PluginChainComponent.cpp
    Source/PluginChainComponent.h
    Source/AudioInputManager.cpp
    Source/AudioInputManager.h
    Source/UserConfig.cpp
    Source/UserConfig.h
)

# Link JUCE modules
target_link_libraries(AudioChain PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Compiler definitions
target_compile_definitions(AudioChain PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AudioChain,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AudioChain,JUCE_VERSION>"
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST=1
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(AudioChain PRIVATE
        JUCE_COREAUDIO=1
        JUCE_COREIMAGE=1
    )
elseif(WIN32)
    target_compile_definitions(AudioChain PRIVATE
        JUCE_DIRECTSOUND=1
        JUCE_WASAPI=1
        JUCE_ASIO=1
        JUCE_USE_WINRT_MIDI=1
    )
    
    # Windows-specific compiler settings
    if(MSVC)
        target_compile_options(AudioChain PRIVATE
            /bigobj          # Handle large object files
            /W3              # Warning level 3
            /wd4996          # Disable deprecation warnings
        )
        
        # Enable multicore compilation
        set_target_properties(AudioChain PROPERTIES
            VS_GLOBAL_UseMultiCoreCompilation "true"
            VS_GLOBAL_EnforceProcessCountAcrossBuilds "true"
        )
    endif()
endif()

# VST2 SDK support (if available)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK_2.4")
    target_include_directories(AudioChain PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/VST_SDK_2.4
    )
    target_compile_definitions(AudioChain PRIVATE
        JUCE_PLUGINHOST_VST=1
    )
endif() 