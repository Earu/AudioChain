#pragma once

#include <JuceHeader.h>
#include "AudioInputManager.h"
#include "VST3PluginHost.h"
#include "AudioProcessor.h"
#include "PluginChainComponent.h"

//==============================================================================
// Custom LookAndFeel for dark theme with sharp corners
class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel()
    {
        // Set dark color scheme
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(0xff1a1a1a));
        
        // ComboBox colors
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d2d));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff404040));
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
        
        // PopupMenu colors
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2d2d2d));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff404040));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }
    
    // Override to remove rounded corners from buttons and provide larger font for CTA
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                         .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        g.setColour(baseColour);
        g.fillRect(bounds); // Sharp corners instead of fillRoundedRectangle
        
        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRect(bounds, 1.0f); // Sharp corners border
    }
    
    // Override to provide larger font for buttons (especially CTA)
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        // Check if this is likely the CTA button (wider than normal buttons)
        if (button.getWidth() > 150)
            return juce::Font(16.0f, juce::Font::bold); // Larger, bold font for CTA
        else
            return juce::Font(14.0f); // Normal font for other buttons
    }
    
    // Override to remove rounded corners from ComboBox
    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                     int, int, int, int, juce::ComboBox& box) override
    {
        auto cornerSize = 0.0f; // Sharp corners
        juce::Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRect(boxBounds); // Sharp corners

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRect(boxBounds, 1); // Sharp corners border

        juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));

        // Draw arrow
        auto arrowX = arrowZone.getCentreX();
        auto arrowY = arrowZone.getCentreY() - 2;
        
        juce::Path path;
        path.startNewSubPath(arrowX - 3.0f, arrowY);
        path.lineTo(arrowX, arrowY + 3.0f);
        path.lineTo(arrowX + 3.0f, arrowY);
        
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::AudioIODeviceCallback
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    void timerCallback() override;

private:
    //==============================================================================
    // Custom LookAndFeel
    DarkLookAndFeel darkLookAndFeel;
    
    // Audio Input Manager
    std::unique_ptr<AudioInputManager> audioInputManager;
    
    // Audio Processing Chain
    std::unique_ptr<AudioProcessor> audioProcessor;
    
    // VST3 Plugin Host
    std::unique_ptr<VST3PluginHost> pluginHost;
    
    // UI Components
    std::unique_ptr<PluginChainComponent> pluginChainComponent;
    
    // Controls
    juce::ComboBox inputDeviceComboBox;
    juce::Label inputDeviceLabel;
    juce::ComboBox outputDeviceComboBox;
    juce::Label outputDeviceLabel;
    juce::TextButton processingToggleButton;
    
    // Input level meters
    juce::Label leftLevelLabel;
    juce::Label rightLevelLabel;
    
    // Status
    bool isProcessingActive = false;
    
    // Level meter bounds (set by setupLayout, used by paint)
    juce::Rectangle<int> leftMeterBounds;
    juce::Rectangle<int> rightMeterBounds;
    
    // Status indicator bounds
    juce::Rectangle<int> inputStatusIndicatorBounds;
    juce::Rectangle<int> outputStatusIndicatorBounds;
    
    // Layout
    void setupLayout();
    void updateInputDeviceList();
    
    // Callbacks
    void toggleProcessing();
    void inputDeviceChanged();
    void outputDeviceChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
}; 