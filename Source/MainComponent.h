#pragma once

#include "AudioInputManager.h"
#include "AudioProcessor.h"
#include "PluginChainComponent.h"
#include "UserConfig.h"
#include "PluginHost.h"
#include <JuceHeader.h>

//==============================================================================
// Enhanced Modern Dark LookAndFeel with gradients and visual effects
class DarkLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    DarkLookAndFeel() {
        // Enhanced dark color scheme with modern colors
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0f0f0f));
        setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(0xff0f0f0f));

        // ComboBox colors with darker styling to match plastic header
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141414));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0e0e0));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff303030));
        setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

        // PopupMenu colors - clean monochromatic theme
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1e1e1e));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe0e0e0));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::white.withAlpha(0.15f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        // Remove any additional borders or outlines
        setColour(juce::PopupMenu::headerTextColourId, juce::Colours::white);
        setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(0xff1e1e1e));
    }

    // Enhanced button drawing with gradients and effects
    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();

        // Check if this is the CTA button (processing toggle)
        bool isCTAButton = button.getWidth() > 150;

        if (isCTAButton) {
            // Modern CTA button with subtle gradient for visual flair
            juce::Colour baseColour = backgroundColour;

            // Clean skeumorphic gradient similar to other buttons
            juce::ColourGradient gradient;
            if (shouldDrawButtonAsDown) {
                // Pressed state - inverted gradient for "pushed in" effect
                gradient = juce::ColourGradient(baseColour.darker(0.2f), bounds.getX(), bounds.getY(),
                                                baseColour.brighter(0.1f), bounds.getX(), bounds.getBottom(), false);
            } else if (shouldDrawButtonAsHighlighted) {
                // Highlighted state - enhanced gradient
                gradient = juce::ColourGradient(baseColour.brighter(0.2f), bounds.getX(), bounds.getY(),
                                                baseColour.darker(0.1f), bounds.getX(), bounds.getBottom(), false);
            } else {
                // Normal state - clean skeumorphic gradient
                gradient = juce::ColourGradient(baseColour.brighter(0.1f), bounds.getX(), bounds.getY(),
                                                baseColour.darker(0.1f), bounds.getX(), bounds.getBottom(), false);
            }

            // Fill with clean gradient like other buttons
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, 4.0f);

            // Add subtle glow effect when highlighted
            if (shouldDrawButtonAsHighlighted) {
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.drawRoundedRectangle(bounds.expanded(1), 5.0f, 1.0f);
            }

            // Clean border like other buttons
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        } else {
            // Regular buttons with subtle gradient
            auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                  .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

            if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
                baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

            juce::ColourGradient gradient(baseColour.brighter(0.1f), bounds.getX(), bounds.getY(),
                                          baseColour.darker(0.1f), bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(bounds, 2.0f);

            g.setColour(button.findColour(juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        }
    }

    // Enhanced ComboBox with modern styling
    void drawComboBox(juce::Graphics &g, int width, int height, bool, int, int, int, int,
                      juce::ComboBox &box) override {
        juce::Rectangle<float> boxBounds(0, 0, width, height);

        // Darker gradient background to match plastic header
        juce::ColourGradient gradient(juce::Colour(0xff1a1a1a), 0, 0, juce::Colour(0xff141414), 0, height, false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(boxBounds, 4.0f);

        // Modern border with white accent
        g.setColour(box.hasKeyboardFocus(true) ? juce::Colours::white : juce::Colour(0xff303030));
        g.drawRoundedRectangle(boxBounds, 4.0f, 1.0f);

        // Enhanced arrow with modern styling
        juce::Rectangle<float> arrowZone(width - 30, 0, 20, height);
        g.setColour(juce::Colours::white.withAlpha(box.isEnabled() ? 0.9f : 0.3f));

        auto arrowX = arrowZone.getCentreX();
        auto arrowY = arrowZone.getCentreY() - 1;

        juce::Path path;
        path.startNewSubPath(arrowX - 4.0f, arrowY - 2.0f);
        path.lineTo(arrowX, arrowY + 2.0f);
        path.lineTo(arrowX + 4.0f, arrowY - 2.0f);

        g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    }

    // Clean popup menu styling to match monochromatic theme
    void drawPopupMenuBackground(juce::Graphics &g, int width, int height) override {
        juce::Rectangle<float> bounds(0, 0, width, height);

        // Flat dark background - no gradient for cleaner look
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRoundedRectangle(bounds, 4.0f); // Smaller radius to match other elements

        // Single clean white border
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    // Override to provide larger font for buttons (especially CTA)
    juce::Font getTextButtonFont(juce::TextButton &button, int buttonHeight) override {
        // Check if this is the close button (contains X symbol)
        if (button.getButtonText() == "X")
            return juce::Font(18.0f, juce::Font::bold); // Larger font for close button
        // Check if this is the minimize button (contains − symbol)
        else if (button.getButtonText() == "−")
            return juce::Font(18.0f, juce::Font::bold); // Larger font for minimize button
        // Check if this is likely the CTA button (wider than normal buttons)
        else if (button.getWidth() > 150)
            return juce::Font(16.0f, juce::Font::bold); // Larger, bold font for CTA
        else
            return juce::Font(14.0f); // Normal font for other buttons
    }

    // Override to provide pointer cursor for buttons
    juce::MouseCursor getMouseCursorFor(juce::Component &component) override {
        if (dynamic_cast<juce::TextButton *>(&component) != nullptr)
            return juce::MouseCursor::PointingHandCursor;

        return juce::LookAndFeel_V4::getMouseCursorFor(component);
    }

    // Override to draw custom icons for the processing button
    void drawButtonText(juce::Graphics &g, juce::TextButton &button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        // Check if this is the processing toggle button (empty text and square)
        if (button.getButtonText().isEmpty() && button.getWidth() == button.getHeight()) {
            auto bounds = button.getLocalBounds().toFloat().reduced(6);
            bool isActive = button.getToggleState();

            if (isActive) {
                // Draw stop icon (square)
                g.setColour(juce::Colours::black);
                g.fillRect(bounds.reduced(4));
            } else {
                // Draw play icon (triangle)
                g.setColour(juce::Colours::black); // Dark icon on light background
                juce::Path playIcon;
                auto center = bounds.getCentre();
                auto size = bounds.getWidth() * 0.6f; // Increased from 0.4f to 0.6f for fatter arrow

                playIcon.addTriangle(center.x - size/2, center.y - size/2,
                                   center.x - size/2, center.y + size/2,
                                   center.x + size/2, center.y);
                g.fillPath(playIcon);
            }
        } else {
            // Use default text drawing for other buttons
            juce::LookAndFeel_V4::drawButtonText(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component, public juce::Timer, public juce::AudioIODeviceCallback {
  public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData, int numInputChannels,
                                          float *const *outputChannelData, int numOutputChannels, int numSamples,
                                          const juce::AudioIODeviceCallbackContext &context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
    void audioDeviceStopped() override;

    //==============================================================================
    void paint(juce::Graphics &g) override;
    void resized() override;

    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    // Mouse events for window dragging
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;

  private:
    //==============================================================================
    // Custom LookAndFeel
    DarkLookAndFeel darkLookAndFeel;

    // Audio Input Manager
    std::unique_ptr<AudioInputManager> audioInputManager;

    // Audio Processing Chain
    std::unique_ptr<AudioProcessor> audioProcessor;

    // VST3 Plugin Host
    std::unique_ptr<PluginHost> pluginHost;

    // User Configuration
    std::unique_ptr<UserConfig> userConfig;

    // UI Components
    std::unique_ptr<PluginChainComponent> pluginChainComponent;

    // Controls
    juce::ComboBox inputDeviceComboBox;
    juce::Label inputDeviceLabel;
    juce::ComboBox outputDeviceComboBox;
    juce::Label outputDeviceLabel;
    juce::TextButton processingToggleButton;
    juce::TextButton closeButton;
    juce::TextButton minimizeButton;
    juce::Label titleLabel;

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

    // Header area bounds for window dragging
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> titleBounds; // For engraved title effect
    juce::ComponentDragger windowDragger;

    // Layout
    void setupLayout();
    void updateInputDeviceList();

    // Enhanced visual methods
    void drawEnhancedLevelMeter(juce::Graphics &g, const juce::Rectangle<int> &bounds, float level);
    void drawTechGrid(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawTechStatusIndicator(juce::Graphics &g, const juce::Rectangle<int> &bounds, bool isActive);

    // Callbacks
    void toggleProcessing();
    void inputDeviceChanged();
    void outputDeviceChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};