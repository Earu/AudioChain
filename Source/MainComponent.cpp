#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
    DBG("MainComponent constructor starting...");

    // Apply custom dark theme LookAndFeel
    setLookAndFeel(&darkLookAndFeel);

    // Initialize core components first - test each one individually
    DBG("Creating AudioInputManager...");
    audioInputManager = std::make_unique<AudioInputManager>();
    DBG("AudioInputManager created successfully");

    DBG("Creating AudioProcessor...");
    audioProcessor = std::make_unique<AudioProcessor>();
    DBG("AudioProcessor created successfully");

    DBG("Creating UserConfig...");
    userConfig = std::make_unique<UserConfig>();
    DBG("UserConfig created successfully");

    DBG("Creating PluginHost...");
    pluginHost = std::make_unique<PluginHost>();
    pluginHost->setUserConfig(userConfig.get());
    DBG("PluginHost created successfully");

    DBG("Creating PluginChainComponent...");
    pluginChainComponent = std::make_unique<PluginChainComponent>(*pluginHost);
    pluginChainComponent->getPluginBrowser()->setUserConfig(userConfig.get());
    DBG("PluginChainComponent created successfully");

    // Initialize UI components - this is required for setupLayout to work
    DBG("Adding UI components to view...");
    addAndMakeVisible(inputDeviceLabel);
    addAndMakeVisible(inputDeviceComboBox);
    addAndMakeVisible(outputDeviceLabel);
    addAndMakeVisible(outputDeviceComboBox);
    addAndMakeVisible(processingToggleButton);
    addAndMakeVisible(closeButton);
    addAndMakeVisible(minimizeButton);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(leftLevelLabel);
    addAndMakeVisible(rightLevelLabel);
    addAndMakeVisible(*pluginChainComponent);
    DBG("UI components added successfully");

    // Set basic text for components with modern styling
    DBG("Setting inputDeviceLabel text and properties...");
    inputDeviceLabel.setText("Input Device:", juce::dontSendNotification);
    inputDeviceLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    inputDeviceLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    DBG("inputDeviceLabel configured successfully");

    DBG("Setting outputDeviceLabel text and properties...");
    outputDeviceLabel.setText("Output Device:", juce::dontSendNotification);
    outputDeviceLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    outputDeviceLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    DBG("outputDeviceLabel configured successfully");

    DBG("Setting leftLevelLabel text and properties...");
    leftLevelLabel.setText("L", juce::dontSendNotification);
    leftLevelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    leftLevelLabel.setJustificationType(juce::Justification::centred);
    leftLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    DBG("leftLevelLabel configured successfully");

    DBG("Setting rightLevelLabel text and properties...");
    rightLevelLabel.setText("R", juce::dontSendNotification);
    rightLevelLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    rightLevelLabel.setJustificationType(juce::Justification::centred);
    rightLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    DBG("rightLevelLabel configured successfully");

    // Configure buttons with modern styling

    // Modern square icon button styling for processing toggle - lighter to show it's interactive
    processingToggleButton.setButtonText(""); // No text, we'll draw custom icons
    processingToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe0e0e0)); // Light gray, close to white
    processingToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00ff88)); // Green when active
    processingToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    processingToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    // Modern close button styling
    closeButton.setButtonText("X");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4444).withAlpha(0.3f));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
    closeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    closeButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

    // Modern minimize button styling
    minimizeButton.setButtonText("_");
    minimizeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    minimizeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff666666).withAlpha(0.3f));
    minimizeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
    minimizeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    minimizeButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

    // App title styling - make it transparent so we can draw the engraved effect manually
    titleLabel.setText("AudioChain", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::transparentBlack); // Hide default text
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    // Enhanced combo boxes with modern dark theme
    inputDeviceComboBox.setTextWhenNothingSelected("Select input device...");
    inputDeviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e1e1e));
    inputDeviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0e0e0));
    inputDeviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    inputDeviceComboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a));
    inputDeviceComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

    outputDeviceComboBox.setTextWhenNothingSelected("Select output device...");
    outputDeviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e1e1e));
    outputDeviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0e0e0));
    outputDeviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    outputDeviceComboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a));
    outputDeviceComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

    // Apply dark theme to labels
    inputDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    outputDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    leftLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    rightLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Set up callbacks
    processingToggleButton.onClick = [this] { toggleProcessing(); };
    inputDeviceComboBox.onChange = [this] { inputDeviceChanged(); };
    outputDeviceComboBox.onChange = [this] { outputDeviceChanged(); };
    closeButton.onClick = [this] {
        DBG("Close button clicked - shutting down application");
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    };
    minimizeButton.onClick = [this] {
        DBG("Minimize button clicked");
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>()) {
            window->minimiseButtonPressed();
        }
    };

    // Layout the components
    setupLayout();

    // Start timer for status updates
    startTimer(100);

    // Populate device list after a short delay to ensure everything is initialized
    juce::Timer::callAfterDelay(500, [this]() {
        DBG("Populating device list...");
        updateInputDeviceList();
    });

    DBG("MainComponent constructor finished");

// Check microphone permissions on macOS
#if JUCE_MAC
    DBG("Checking macOS microphone permissions...");
    // Note: In a real app, you'd use proper permission checking APIs
    // For now, this is just a reminder to check system preferences
    DBG("Please ensure AudioChain has microphone permissions in System Settings → Privacy & Security → Microphone");
#elif JUCE_WINDOWS
    DBG("Checking Windows microphone permissions...");
    DBG("Please ensure AudioChain has microphone permissions in Settings → Privacy → Microphone");
#endif

    // Set window size
    setSize(800, 700);
}

MainComponent::~MainComponent() {
    stopTimer();

    // Remove ourselves as audio callback and stop processing
    if (audioInputManager && isProcessingActive) {
        audioInputManager->getAudioDeviceManager().removeAudioCallback(this);
        audioInputManager->stop();

        if (audioProcessor)
            audioProcessor->stop();
    }

    // Clean up custom LookAndFeel
    setLookAndFeel(nullptr);
}

//==============================================================================
// AudioIODeviceCallback implementation
void MainComponent::audioDeviceIOCallbackWithContext(const float *const *inputChannelData, int numInputChannels,
                                                     float *const *outputChannelData, int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext &context) {
    static int callbackCount = 0;
    if (++callbackCount % 1000 == 0) // Log every 1000 callbacks to avoid spam
    {
        DBG("Audio callback #" + juce::String(callbackCount) + " - Input channels: " + juce::String(numInputChannels) +
            ", Output channels: " + juce::String(numOutputChannels) + ", Samples: " + juce::String(numSamples) +
            ", inputChannelData: " + juce::String(inputChannelData ? "valid" : "null") +
            ", isProcessingActive: " + juce::String(isProcessingActive ? "true" : "false"));
    }

    // Clear output buffers first
    for (int channel = 0; channel < numOutputChannels; ++channel) {
        if (outputChannelData[channel])
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
    }

    // Update input levels for metering
    if (audioInputManager && isProcessingActive && inputChannelData) {
        audioInputManager->updateInputLevels(inputChannelData, numInputChannels, numSamples);

        // Debug: Check if we actually have audio signal every 2000 callbacks
        if (callbackCount % 2000 == 0) {
            float maxSample = 0.0f;
            float sumSamples = 0.0f;
            int nonZeroSamples = 0;

            for (int channel = 0; channel < numInputChannels; ++channel) {
                if (inputChannelData[channel]) {
                    for (int sample = 0; sample < numSamples; ++sample) {
                        float sampleValue = inputChannelData[channel][sample];
                        float absSample = std::abs(sampleValue);
                        maxSample = juce::jmax(maxSample, absSample);
                        sumSamples += absSample;
                        if (absSample > 0.000001f)
                            nonZeroSamples++;

                        // Log first few samples for debugging
                        if (sample < 5 && channel == 0) {
                            DBG("Sample[" + juce::String(sample) + "] = " + juce::String(sampleValue, 8));
                        }
                    }
                } else {
                    DBG("Channel " + juce::String(channel) + " inputChannelData is NULL!");
                }
            }

            float averageLevel =
                (numInputChannels * numSamples > 0) ? sumSamples / (numInputChannels * numSamples) : 0.0f;

            DBG("Audio analysis - Max: " + juce::String(maxSample, 6) + " (" +
                juce::String(juce::Decibels::gainToDecibels(maxSample, -60.0f), 1) + "dB)" +
                ", Avg: " + juce::String(averageLevel, 6) + ", Non-zero samples: " + juce::String(nonZeroSamples) +
                "/" + juce::String(numInputChannels * numSamples));
        }
    }

    // Process audio if we're active (removed numOutputChannels > 0 requirement since we have 0 output channels)
    if (isProcessingActive && inputChannelData && numInputChannels > 0) {
        // Create a temporary buffer for processing - ensure we have at least 2 channels for stereo processing
        int processingChannels = juce::jmax(juce::jmax(numInputChannels, numOutputChannels), 2);
        juce::AudioBuffer<float> processingBuffer(processingChannels, numSamples);

        // Copy input to processing buffer
        for (int channel = 0; channel < numInputChannels && channel < processingBuffer.getNumChannels(); ++channel) {
            if (inputChannelData[channel]) {
                processingBuffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
            }
        }

        // Handle mono-to-stereo conversion: if we only have 1 input channel, duplicate it to channel 1
        if (numInputChannels == 1 && processingBuffer.getNumChannels() >= 2) {
            if (inputChannelData[0]) {
                processingBuffer.copyFrom(1, 0, inputChannelData[0], numSamples);
                DBG("Duplicating mono input to stereo for processing");
            }
        }

        // Process through VST plugins
        if (pluginHost) {
            pluginHost->processAudio(processingBuffer);
        }

        // Process through our audio processor
        if (audioProcessor) {
            audioProcessor->processAudio(processingBuffer);
        }

        // Copy processed audio to output
        for (int channel = 0; channel < numOutputChannels && channel < processingBuffer.getNumChannels(); ++channel) {
            if (outputChannelData[channel]) {
                juce::FloatVectorOperations::copy(outputChannelData[channel], processingBuffer.getReadPointer(channel),
                                                  numSamples);
            }
        }
    }
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    DBG("Audio device about to start: " + device->getName());

    double sampleRate = device->getCurrentSampleRate();
    int bufferSize = device->getCurrentBufferSizeSamples();

    // Prepare audio processor
    if (audioProcessor) {
        audioProcessor->prepareToPlay(bufferSize, sampleRate);
    }

    // Prepare plugin host
    if (pluginHost) {
        pluginHost->prepareToPlay(bufferSize, sampleRate);
    }

    // Configure audio input manager
    if (audioInputManager) {
        audioInputManager->setSampleRate(sampleRate);
        audioInputManager->setBufferSize(bufferSize);
    }

    DBG("Audio prepared - Sample rate: " + juce::String(sampleRate) + ", Buffer size: " + juce::String(bufferSize));
}

void MainComponent::audioDeviceStopped() {
    DBG("Audio device stopped");

    if (audioProcessor)
        audioProcessor->releaseResources();

    if (pluginHost)
        pluginHost->releaseResources();
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds();

    // Ultra-modern flat background - deep space black
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Enhanced header with realistic plastic/amp-style background
    juce::Rectangle<int> headerArea(0, 0, getWidth(), 110);

    // Realistic plastic gradient background
    juce::ColourGradient plasticGradient(
        juce::Colour(0xff3a3a3a), headerArea.getX(), headerArea.getY(),
        juce::Colour(0xff1a1a1a), headerArea.getX(), headerArea.getBottom(),
        false
    );

    // Add subtle color variation for more realistic plastic look
    plasticGradient.addColour(0.3, juce::Colour(0xff2e2e2e));
    plasticGradient.addColour(0.7, juce::Colour(0xff252525));

    g.setGradientFill(plasticGradient);
    g.fillRect(headerArea);

    // Add grain texture for realistic plastic surface
    juce::Random grainRandom(42); // Fixed seed for consistent grain
    g.setColour(juce::Colours::white.withAlpha(0.02f));

    for (int i = 0; i < headerArea.getWidth() * headerArea.getHeight() / 8; ++i) {
        int x = grainRandom.nextInt(headerArea.getWidth());
        int y = grainRandom.nextInt(headerArea.getHeight());
        g.fillRect(x, y, 1, 1);
    }

    // Add darker grain for depth
    grainRandom.setSeed(84);
    g.setColour(juce::Colours::black.withAlpha(0.03f));

    for (int i = 0; i < headerArea.getWidth() * headerArea.getHeight() / 12; ++i) {
        int x = grainRandom.nextInt(headerArea.getWidth());
        int y = grainRandom.nextInt(headerArea.getHeight());
        g.fillRect(x, y, 1, 1);
    }

    // Modern header border with white accent - thicker and more prominent
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawLine(0, 110, getWidth(), 110, 3);

    // Add subtle accent line at top
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(0, 0, getWidth(), 0, 1);

        // Draw engraved title effect
    if (!titleBounds.isEmpty()) {
        // Use a more stylized font - try different typefaces for better aesthetics
        juce::Font titleFont("Arial Black", 16.0f, juce::Font::bold);
        g.setFont(titleFont);

        // Draw shadow (darker, offset down and right)
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawText("AudioChain", titleBounds.translated(1, 1), juce::Justification::centredLeft);

        // Draw highlight (lighter, offset up and left)
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawText("AudioChain", titleBounds.translated(-1, -1), juce::Justification::centredLeft);

        // Draw main text (medium tone, centered)
        g.setColour(juce::Colour(0xff888888));
        g.drawText("AudioChain", titleBounds, juce::Justification::centredLeft);
    }

    // Modern tech-style status indicators
    if (!inputStatusIndicatorBounds.isEmpty()) {
        drawTechStatusIndicator(g, inputStatusIndicatorBounds, isProcessingActive);
    }

    if (!outputStatusIndicatorBounds.isEmpty()) {
        drawTechStatusIndicator(g, outputStatusIndicatorBounds, isProcessingActive);
    }

    // Enhanced level meters with modern styling
    drawEnhancedLevelMeter(g, leftMeterBounds, audioInputManager ? audioInputManager->getInputLevel(0) : 0.0f);
    drawEnhancedLevelMeter(g, rightMeterBounds, audioInputManager ? audioInputManager->getInputLevel(1) : 0.0f);
}

void MainComponent::drawEnhancedLevelMeter(juce::Graphics &g, const juce::Rectangle<int> &bounds, float level) {
    if (bounds.isEmpty())
        return;

    auto meterBounds = bounds.toFloat();

    // Ultra-modern flat meter background - deep black
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(meterBounds);

    // Sharp, modern border with white accent
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRect(meterBounds, 1.0f);

    if (isProcessingActive && level > 0.0f) {
        // Convert to dB and normalize
        float levelDb = juce::Decibels::gainToDecibels(level, -60.0f);
        float normalizedLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
        normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);

        if (normalizedLevel > 0.0f) {
            auto fillArea = meterBounds.reduced(2);
            float fillHeight = fillArea.getHeight() * normalizedLevel;
            auto fillRect = juce::Rectangle<float>(fillArea.getX(), fillArea.getBottom() - fillHeight,
                                                   fillArea.getWidth(), fillHeight);

            // Modern monochromatic colors based on level - white/gray scale with minimal color
            juce::Colour levelColour;
            if (normalizedLevel > 0.85f) {
                // Danger zone - subtle red tint
                levelColour = juce::Colour(0xffff6666);
            } else if (normalizedLevel > 0.7f) {
                // Warning zone - light gray
                levelColour = juce::Colour(0xffcccccc);
            } else if (normalizedLevel > 0.5f) {
                // Moderate zone - medium gray
                levelColour = juce::Colour(0xffaaaaaa);
            } else {
                // Safe zone - white
                levelColour = juce::Colours::white;
            }

            // Apply flat fill - sharp, modern look
            g.setColour(levelColour);
            g.fillRect(fillRect);

            // Add white accent border for high levels
            if (normalizedLevel > 0.7f) {
                g.setColour(juce::Colours::white.withAlpha(0.4f));
                g.drawRect(fillRect, 1.0f);
            }
        }
    }

    // Enhanced scale marks with modern styling
    g.setColour(juce::Colour(0xff666666).withAlpha(0.7f));
    std::vector<float> dbMarks = {-60.0f, -40.0f, -20.0f, -10.0f, -5.0f, 0.0f};

    for (float db : dbMarks) {
        float normalizedPos = juce::jmap(db, -60.0f, 0.0f, 1.0f, 0.0f);
        int yPos = meterBounds.getY() + (int)(meterBounds.getHeight() * normalizedPos);

        // Draw modern tick marks
        g.setColour(db == 0.0f ? juce::Colours::white : juce::Colour(0xff666666));
        g.drawLine(meterBounds.getRight() + 2, yPos, meterBounds.getRight() + 8, yPos, 1.5f);

        // Add dB labels for key marks
        if (db == 0.0f || db == -20.0f || db == -40.0f) {
            g.setFont(juce::Font(9.0f));
            g.setColour(juce::Colour(0xff999999));
            juce::String dbText = (db == 0.0f) ? "0" : juce::String((int)db);
            g.drawText(dbText, meterBounds.getRight() + 10, yPos - 6, 20, 12, juce::Justification::centredLeft);
        }
    }
}

void MainComponent::drawTechGrid(juce::Graphics &g, const juce::Rectangle<int> &area) {
    // Modern tech grid pattern with subtle lines and accent highlights

    // Grid parameters for a sleek, tech aesthetic
    const int gridSize = 20; // Size of each grid cell
    const float lineThickness = 0.5f;
    const float accentLineThickness = 1.0f;

    // Base grid color - very subtle
    g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));

    // Draw vertical grid lines
    for (int x = area.getX(); x <= area.getRight(); x += gridSize) {
        // Every 5th line is slightly more prominent
        bool isAccentLine = ((x - area.getX()) / gridSize) % 5 == 0;

        if (isAccentLine) {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawLine(x, area.getY(), x, area.getBottom(), accentLineThickness);
        } else {
            g.setColour(juce::Colour(0xff404040).withAlpha(0.2f));
            g.drawLine(x, area.getY(), x, area.getBottom(), lineThickness);
        }
    }

    // Draw horizontal grid lines
    for (int y = area.getY(); y <= area.getBottom(); y += gridSize) {
        // Every 5th line is slightly more prominent
        bool isAccentLine = ((y - area.getY()) / gridSize) % 5 == 0;

        if (isAccentLine) {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawLine(area.getX(), y, area.getRight(), y, accentLineThickness);
        } else {
            g.setColour(juce::Colour(0xff404040).withAlpha(0.2f));
            g.drawLine(area.getX(), y, area.getRight(), y, lineThickness);
        }
    }

    // Add some tech-inspired accent elements

    // Corner accent markers - small tech-style corner brackets
    auto cornerSize = 8;
    auto cornerThickness = 2.0f;
    g.setColour(juce::Colours::white.withAlpha(0.3f));

    // Top-left corner
    g.drawLine(area.getX() + 5, area.getY() + 5, area.getX() + 5 + cornerSize, area.getY() + 5, cornerThickness);
    g.drawLine(area.getX() + 5, area.getY() + 5, area.getX() + 5, area.getY() + 5 + cornerSize, cornerThickness);

    // Top-right corner
    g.drawLine(area.getRight() - 5, area.getY() + 5, area.getRight() - 5 - cornerSize, area.getY() + 5,
               cornerThickness);
    g.drawLine(area.getRight() - 5, area.getY() + 5, area.getRight() - 5, area.getY() + 5 + cornerSize,
               cornerThickness);

    // Bottom-left corner
    g.drawLine(area.getX() + 5, area.getBottom() - 5, area.getX() + 5 + cornerSize, area.getBottom() - 5,
               cornerThickness);
    g.drawLine(area.getX() + 5, area.getBottom() - 5, area.getX() + 5, area.getBottom() - 5 - cornerSize,
               cornerThickness);

    // Bottom-right corner
    g.drawLine(area.getRight() - 5, area.getBottom() - 5, area.getRight() - 5 - cornerSize, area.getBottom() - 5,
               cornerThickness);
    g.drawLine(area.getRight() - 5, area.getBottom() - 5, area.getRight() - 5, area.getBottom() - 5 - cornerSize,
               cornerThickness);

    // Add subtle scan line effect
    static float scanLineOffset = 0.0f;
    scanLineOffset += 0.5f; // Animate the scan line
    if (scanLineOffset > area.getHeight())
        scanLineOffset = 0.0f;

    g.setColour(juce::Colours::white.withAlpha(0.03f));
    g.drawLine(area.getX(), area.getY() + scanLineOffset, area.getRight(), area.getY() + scanLineOffset, 1.0f);
}

void MainComponent::drawTechStatusIndicator(juce::Graphics &g, const juce::Rectangle<int> &bounds, bool isActive) {
    auto indicatorBounds = bounds.toFloat();

    if (isActive) {
        // Active state - modern tech style with sharp edges

        // Outer ring - white
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawEllipse(indicatorBounds.expanded(1), 1.5f);

        // Inner core - bright green for device status
        g.setColour(juce::Colour(0xff00ff88));
        g.fillEllipse(indicatorBounds.reduced(1));

        // Center dot - white for high contrast
        g.setColour(juce::Colours::white);
        g.fillEllipse(indicatorBounds.reduced(3));

        // Add subtle pulsing effect with corner accents
        auto centerX = indicatorBounds.getCentreX();
        auto centerY = indicatorBounds.getCentreY();
        auto radius = indicatorBounds.getWidth() * 0.6f;

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        // Draw small accent lines at cardinal directions
        g.drawLine(centerX - radius, centerY, centerX - radius + 3, centerY, 1.0f);
        g.drawLine(centerX + radius - 3, centerY, centerX + radius, centerY, 1.0f);
        g.drawLine(centerX, centerY - radius, centerX, centerY - radius + 3, 1.0f);
        g.drawLine(centerX, centerY + radius - 3, centerX, centerY + radius, 1.0f);
    } else {
        // Inactive state - minimal, dark

        // Dark background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillEllipse(indicatorBounds);

        // Subtle border
        g.setColour(juce::Colour(0xff404040));
        g.drawEllipse(indicatorBounds, 1.0f);

        // Small center dot to show it's a status indicator
        g.setColour(juce::Colour(0xff666666));
        g.fillEllipse(indicatorBounds.reduced(3));
    }
}

void MainComponent::resized() { setupLayout(); }

void MainComponent::setupLayout() {
    auto area = getLocalBounds();

    // Header area - compact layout
    auto headerArea = area.removeFromTop(110); // Reduced from 140 to 110 for more compact layout
    headerBounds = headerArea; // Store for window dragging

        // Window controls area at the top
    auto windowControlsArea = headerArea.removeFromTop(30);

    // Title on the left side of window controls
    auto titleArea = juce::Rectangle<int>(10, 5, 200, 25);
    titleLabel.setBounds(titleArea);
    titleBounds = titleArea; // Store for engraved drawing

    // Window control buttons on the right
    auto closeButtonArea = juce::Rectangle<int>(getWidth() - 45, 5, 35, 25);
    closeButton.setBounds(closeButtonArea);

    auto minimizeButtonArea = juce::Rectangle<int>(getWidth() - 85, 5, 35, 25);
    minimizeButton.setBounds(minimizeButtonArea);

    // Device selection area (labels + dropdowns + status indicators) - fixed height
    auto deviceArea = headerArea.removeFromTop(70); // Fixed height instead of using all remaining space
    deviceArea.removeFromTop(5); // Small top padding

    // Processing button on the far right (fixed width)
    auto buttonSize = 30;
    auto processingArea = deviceArea.removeFromRight(buttonSize + 10); // Button + padding

    // Split remaining device area equally between input and output
    auto inputDeviceArea = deviceArea.removeFromLeft(deviceArea.getWidth() / 2);
    auto outputDeviceArea = deviceArea; // Remaining space

    // Input device section (label above dropdown) - more vertical space
    auto inputLabelArea = inputDeviceArea.removeFromTop(25);
    inputDeviceLabel.setBounds(inputLabelArea.reduced(5, 0));

    auto inputControlArea = inputDeviceArea.removeFromTop(35);
    inputDeviceComboBox.setBounds(inputControlArea.removeFromLeft(inputControlArea.getWidth() - 25).reduced(5, 0));

    // Status indicator circle next to input combo box
    inputStatusIndicatorBounds = inputControlArea.reduced(5);
    inputStatusIndicatorBounds = inputStatusIndicatorBounds.withSizeKeepingCentre(10, 10);

    // Output device section (label above dropdown) - more vertical space
    auto outputLabelArea = outputDeviceArea.removeFromTop(25);
    outputDeviceLabel.setBounds(outputLabelArea.reduced(5, 0));

    auto outputControlArea = outputDeviceArea.removeFromTop(35);
    outputDeviceComboBox.setBounds(outputControlArea.removeFromLeft(outputControlArea.getWidth() - 25).reduced(5, 0));

    // Status indicator circle next to output combo box
    outputStatusIndicatorBounds = outputControlArea.reduced(5);
    outputStatusIndicatorBounds = outputStatusIndicatorBounds.withSizeKeepingCentre(10, 10);

    // Processing button section (aligned with device selections)
    auto processingLabelArea = processingArea.removeFromTop(25);
    // No label needed for processing button, just use the space for alignment

    auto processingControlArea = processingArea.removeFromTop(35);
    // Center the square button in the processing area
    auto buttonArea = processingControlArea.withSizeKeepingCentre(buttonSize, buttonSize);
    processingToggleButton.setBounds(buttonArea);

    // Main content area with level meters on the right
    auto contentArea = area.removeFromBottom(area.getHeight() - 10);
    contentArea = contentArea.removeFromLeft(contentArea.getWidth() - 15);

    auto levelMeterArea = contentArea.removeFromRight(80); // Reserve space for level meters

    // Plugin chain area (remaining space)
    pluginChainComponent->setBounds(contentArea);

    // Center the level meters in the available space
    auto meterWidth = 25;                                                  // Width of each meter
    auto meterSpacing = 10;                                                // Space between meters
    auto totalMeterWidth = (meterWidth * 2) + meterSpacing;                // Total width needed for both meters
    auto centerOffset = (levelMeterArea.getWidth() - totalMeterWidth) / 2; // Center offset

    // Create centered meter area
    auto centeredMeterArea = levelMeterArea.withTrimmedLeft(centerOffset).withWidth(totalMeterWidth);

    // Level meter labels (top part of centered area)
    auto labelArea = centeredMeterArea.removeFromTop(20);
    auto leftLabelArea = labelArea.removeFromLeft(meterWidth + meterSpacing / 2);
    auto rightLabelArea = labelArea.removeFromLeft(meterWidth + meterSpacing / 2);

    leftLevelLabel.setBounds(leftLabelArea);
    rightLevelLabel.setBounds(rightLabelArea);

    // Store level meter bar areas for painting (remaining centered space)
    auto leftMeterArea = centeredMeterArea.removeFromLeft(meterWidth);
    centeredMeterArea.removeFromLeft(meterSpacing); // Skip spacing
    leftMeterArea.removeFromBottom(10);
    auto rightMeterArea = centeredMeterArea.removeFromLeft(meterWidth);
    rightMeterArea.removeFromBottom(10);

    leftMeterBounds = leftMeterArea.reduced(2);
    rightMeterBounds = rightMeterArea.reduced(2);
}

//==============================================================================
// Mouse events for window dragging
void MainComponent::mouseDown(const juce::MouseEvent &event) {
    // Check if the click is in the header area
    if (headerBounds.contains(event.getPosition())) {
        // Check if we're not clicking on any interactive controls
        auto localPoint = event.getPosition();

        // Don't drag if clicking on combo boxes, buttons, or status indicators
        if (inputDeviceComboBox.getBounds().contains(localPoint) ||
            outputDeviceComboBox.getBounds().contains(localPoint) ||
            processingToggleButton.getBounds().contains(localPoint) ||
            closeButton.getBounds().contains(localPoint) ||
            minimizeButton.getBounds().contains(localPoint) ||
            inputStatusIndicatorBounds.contains(localPoint) ||
            outputStatusIndicatorBounds.contains(localPoint)) {
            return;
        }

        // Start window dragging
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>()) {
            windowDragger.startDraggingComponent(window, event);
        }
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent &event) {
    // Continue dragging if we started it
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>()) {
        windowDragger.dragComponent(window, event, nullptr);
    }
}

//==============================================================================
void MainComponent::timerCallback() {
    // Update level meter labels (just show L and R, the visual meters show the levels)
    leftLevelLabel.setText("L", juce::dontSendNotification);
    rightLevelLabel.setText("R", juce::dontSendNotification);

    // Repaint to update level meters and status indicator
    repaint();
}

// Status is now shown via visual indicator circle

void MainComponent::updateInputDeviceList() {
    inputDeviceComboBox.clear();
    outputDeviceComboBox.clear();

    if (audioInputManager) {
        // Populate input devices
        auto inputDevices = audioInputManager->getAvailableInputDevices();

        for (int i = 0; i < inputDevices.size(); ++i) {
            inputDeviceComboBox.addItem(inputDevices[i], i + 1);
        }

        if (inputDevices.size() > 0) {
            // Try to select microphone by default instead of BlackHole for testing
            int micIndex = -1;
            for (int i = 0; i < inputDevices.size(); ++i) {
                if (inputDevices[i].containsIgnoreCase("microphone")) {
                    micIndex = i;
                    break;
                }
            }

            if (micIndex >= 0) {
                inputDeviceComboBox.setSelectedItemIndex(micIndex);
                DBG("Auto-selected microphone for testing");
            } else {
                inputDeviceComboBox.setSelectedItemIndex(0);
            }
            inputDeviceChanged(); // Set the selected device
        }

        // Populate output devices
        auto outputDevices = audioInputManager->getAvailableOutputDevices();

        for (int i = 0; i < outputDevices.size(); ++i) {
            outputDeviceComboBox.addItem(outputDevices[i], i + 1);
        }

        if (outputDevices.size() > 0) {
            outputDeviceComboBox.setSelectedItemIndex(0);
            outputDeviceChanged(); // Set the selected device
        }
    }
}

//==============================================================================
void MainComponent::toggleProcessing() {
    if (isProcessingActive) {
        // Stop processing
        // Remove ourselves as the audio callback
        if (audioInputManager) {
            audioInputManager->getAudioDeviceManager().removeAudioCallback(this);
            audioInputManager->stop();
        }

        isProcessingActive = false;
        processingToggleButton.setToggleState(false, juce::dontSendNotification);
        inputDeviceComboBox.setEnabled(true);
        outputDeviceComboBox.setEnabled(true);

        if (audioProcessor)
            audioProcessor->stop();

        DBG("Audio processing stopped");
    } else {
        // Start processing
        if (!audioInputManager || !audioInputManager->hasValidInputDevice()) {
            DBG("No input device selected");
            return;
        }

        if (audioInputManager->start()) {
            // Add ourselves as the audio callback
            audioInputManager->getAudioDeviceManager().addAudioCallback(this);

            isProcessingActive = true;
            processingToggleButton.setToggleState(true, juce::dontSendNotification);
            inputDeviceComboBox.setEnabled(false);
            outputDeviceComboBox.setEnabled(false);

            // Start audio processor
            if (audioProcessor)
                audioProcessor->start();

            DBG("Audio processing started from: " + audioInputManager->getCurrentInputDevice());
        } else {
            DBG("Failed to start audio processing");
        }
    }
}

void MainComponent::inputDeviceChanged() {
    int selectedIndex = inputDeviceComboBox.getSelectedItemIndex();

    DBG("Input device selection changed - index: " + juce::String(selectedIndex));

    if (selectedIndex >= 0 && audioInputManager) {
        juce::String deviceName = inputDeviceComboBox.getItemText(selectedIndex);

        DBG("User selected device: '" + deviceName + "'");

        if (audioInputManager->setInputDevice(deviceName)) {
            DBG("Input device changed to: " + deviceName);
        } else {
            DBG("Failed to set input device: " + deviceName);
        }
    }
}

void MainComponent::outputDeviceChanged() {
    int selectedIndex = outputDeviceComboBox.getSelectedItemIndex();

    DBG("Output device selection changed - index: " + juce::String(selectedIndex));

    if (selectedIndex >= 0 && audioInputManager) {
        juce::String deviceName = outputDeviceComboBox.getItemText(selectedIndex);

        DBG("User selected output device: '" + deviceName + "'");

        if (audioInputManager->setOutputDevice(deviceName)) {
            DBG("Output device changed to: " + deviceName);
        } else {
            DBG("Failed to set output device: " + deviceName);
        }
    }
}