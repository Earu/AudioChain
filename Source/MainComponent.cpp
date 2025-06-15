#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
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
    
    DBG("Creating VST3PluginHost...");
    pluginHost = std::make_unique<VST3PluginHost>();
    DBG("VST3PluginHost created successfully");
    
    DBG("Creating PluginChainComponent...");
    pluginChainComponent = std::make_unique<PluginChainComponent>(*pluginHost);
    DBG("PluginChainComponent created successfully");
    
    // Initialize UI components - this is required for setupLayout to work
    DBG("Adding UI components to view...");
    addAndMakeVisible(inputDeviceLabel);
    addAndMakeVisible(inputDeviceComboBox);
    addAndMakeVisible(outputDeviceLabel);
    addAndMakeVisible(outputDeviceComboBox);
    addAndMakeVisible(processingToggleButton);
    // Remove titleLabel - redundant with window title
    // addAndMakeVisible(titleLabel);
    addAndMakeVisible(leftLevelLabel);
    addAndMakeVisible(rightLevelLabel);
    addAndMakeVisible(*pluginChainComponent);
    DBG("UI components added successfully");
    
    // Set basic text for components
    DBG("Setting inputDeviceLabel text and properties...");
    inputDeviceLabel.setText("Audio Input Device:", juce::dontSendNotification);
    inputDeviceLabel.setFont(juce::Font(14.0f));
    DBG("inputDeviceLabel configured successfully");
    
    DBG("Setting outputDeviceLabel text and properties...");
    outputDeviceLabel.setText("Audio Output Device:", juce::dontSendNotification);
    outputDeviceLabel.setFont(juce::Font(14.0f));
    DBG("outputDeviceLabel configured successfully");
    
    DBG("Setting leftLevelLabel text and properties...");
    leftLevelLabel.setText("L", juce::dontSendNotification);
    leftLevelLabel.setFont(juce::Font(14.0f, juce::Font::FontStyleFlags::bold));
    leftLevelLabel.setJustificationType(juce::Justification::centred);
    DBG("leftLevelLabel configured successfully");
    
    DBG("Setting rightLevelLabel text and properties...");
    rightLevelLabel.setText("R", juce::dontSendNotification);
    rightLevelLabel.setFont(juce::Font(14.0f, juce::Font::FontStyleFlags::bold));
    rightLevelLabel.setJustificationType(juce::Justification::centred);
    DBG("rightLevelLabel configured successfully");
    
    // Configure buttons with dark theme
    
    // Apply modern CTA styling to processing toggle button
    processingToggleButton.setButtonText("Start Processing");
    processingToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff007acc)); // Modern blue CTA
    processingToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff005a99)); // Darker blue when pressed
    processingToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    processingToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    // Note: Font will be set via LookAndFeel override for buttons
    
    // Configure combo boxes with dark theme
    inputDeviceComboBox.setTextWhenNothingSelected("Select an input device...");
    inputDeviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d2d));
    inputDeviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    inputDeviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    inputDeviceComboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff404040));
    inputDeviceComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
    
    outputDeviceComboBox.setTextWhenNothingSelected("Select an output device...");
    outputDeviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d2d));
    outputDeviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    outputDeviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    outputDeviceComboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff404040));
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
    #endif
    
    // Set window size
    setSize(800, 700);
}

MainComponent::~MainComponent()
{
    stopTimer();
    
    // Remove ourselves as audio callback and stop processing
    if (audioInputManager && isProcessingActive)
    {
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
void MainComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                     int numInputChannels,
                                                     float* const* outputChannelData,
                                                     int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext& context)
{
    static int callbackCount = 0;
    if (++callbackCount % 1000 == 0) // Log every 1000 callbacks to avoid spam
    {
        DBG("Audio callback #" + juce::String(callbackCount) + 
            " - Input channels: " + juce::String(numInputChannels) + 
            ", Output channels: " + juce::String(numOutputChannels) + 
            ", Samples: " + juce::String(numSamples) + 
            ", inputChannelData: " + juce::String(inputChannelData ? "valid" : "null") +
            ", isProcessingActive: " + juce::String(isProcessingActive ? "true" : "false"));
    }
    
    // Clear output buffers first
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel])
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
    }
    
    // Update input levels for metering
    if (audioInputManager && isProcessingActive && inputChannelData)
    {
        audioInputManager->updateInputLevels(inputChannelData, numInputChannels, numSamples);
        
        // Debug: Check if we actually have audio signal every 2000 callbacks
        if (callbackCount % 2000 == 0)
        {
            float maxSample = 0.0f;
            float sumSamples = 0.0f;
            int nonZeroSamples = 0;
            
            for (int channel = 0; channel < numInputChannels; ++channel)
            {
                if (inputChannelData[channel])
                {
                    for (int sample = 0; sample < numSamples; ++sample)
                    {
                        float sampleValue = inputChannelData[channel][sample];
                        float absSample = std::abs(sampleValue);
                        maxSample = juce::jmax(maxSample, absSample);
                        sumSamples += absSample;
                        if (absSample > 0.000001f) nonZeroSamples++;
                        
                        // Log first few samples for debugging
                        if (sample < 5 && channel == 0)
                        {
                            DBG("Sample[" + juce::String(sample) + "] = " + juce::String(sampleValue, 8));
                        }
                    }
                }
                else
                {
                    DBG("Channel " + juce::String(channel) + " inputChannelData is NULL!");
                }
            }
            
            float averageLevel = (numInputChannels * numSamples > 0) ? sumSamples / (numInputChannels * numSamples) : 0.0f;
            
            DBG("Audio analysis - Max: " + juce::String(maxSample, 6) + 
                " (" + juce::String(juce::Decibels::gainToDecibels(maxSample, -60.0f), 1) + "dB)" +
                ", Avg: " + juce::String(averageLevel, 6) +
                ", Non-zero samples: " + juce::String(nonZeroSamples) + "/" + juce::String(numInputChannels * numSamples));
        }
    }
    
    // Process audio if we're active (removed numOutputChannels > 0 requirement since we have 0 output channels)
    if (isProcessingActive && inputChannelData && numInputChannels > 0)
    {
        // Create a temporary buffer for processing
        juce::AudioBuffer<float> processingBuffer(juce::jmax(numInputChannels, numOutputChannels), numSamples);
        
        // Copy input to processing buffer
        for (int channel = 0; channel < numInputChannels && channel < processingBuffer.getNumChannels(); ++channel)
        {
            if (inputChannelData[channel])
            {
                processingBuffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
            }
        }
        
        // Process through VST plugins
        if (pluginHost)
        {
            pluginHost->processAudio(processingBuffer);
        }
        
        // Process through our audio processor
        if (audioProcessor)
        {
            audioProcessor->processAudio(processingBuffer);
        }
        
        // Copy processed audio to output
        for (int channel = 0; channel < numOutputChannels && channel < processingBuffer.getNumChannels(); ++channel)
        {
            if (outputChannelData[channel])
            {
                juce::FloatVectorOperations::copy(outputChannelData[channel],
                                                processingBuffer.getReadPointer(channel),
                                                numSamples);
            }
        }
    }
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    DBG("Audio device about to start: " + device->getName());
    
    double sampleRate = device->getCurrentSampleRate();
    int bufferSize = device->getCurrentBufferSizeSamples();
    
    // Prepare audio processor
    if (audioProcessor)
    {
        audioProcessor->prepareToPlay(bufferSize, sampleRate);
    }
    
    // Prepare plugin host  
    if (pluginHost)
    {
        pluginHost->prepareToPlay(bufferSize, sampleRate);
    }
    
    // Configure audio input manager
    if (audioInputManager)
    {
        audioInputManager->setSampleRate(sampleRate);
        audioInputManager->setBufferSize(bufferSize);
    }
    
    DBG("Audio prepared - Sample rate: " + juce::String(sampleRate) + 
        ", Buffer size: " + juce::String(bufferSize));
}

void MainComponent::audioDeviceStopped()
{
    DBG("Audio device stopped");
    
    if (audioProcessor)
        audioProcessor->releaseResources();
    
    if (pluginHost)
        pluginHost->releaseResources();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Dark theme background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Draw header background - slightly lighter dark
    juce::Rectangle<int> headerArea(0, 0, getWidth(), 90);
    g.setColour(juce::Colour(0xff2d2d2d));
    g.fillRect(headerArea);
    
    // Draw header border
    g.setColour(juce::Colour(0xff404040));
    g.drawLine(0, 90, getWidth(), 90, 1);
    
    // Draw status indicator circles
    if (!inputStatusIndicatorBounds.isEmpty())
    {
        g.setColour(isProcessingActive ? juce::Colours::green : juce::Colours::red);
        g.fillEllipse(inputStatusIndicatorBounds.toFloat());
        
        // Add a subtle border
        g.setColour(juce::Colour(0xff404040));
        g.drawEllipse(inputStatusIndicatorBounds.toFloat(), 1.0f);
    }
    
    if (!outputStatusIndicatorBounds.isEmpty())
    {
        g.setColour(isProcessingActive ? juce::Colours::green : juce::Colours::red);
        g.fillEllipse(outputStatusIndicatorBounds.toFloat());
        
        // Add a subtle border
        g.setColour(juce::Colour(0xff404040));
        g.drawEllipse(outputStatusIndicatorBounds.toFloat(), 1.0f);
    }
    
    // Draw vertical level meters background
    g.setColour(juce::Colours::black);
    g.fillRect(leftMeterBounds);
    g.fillRect(rightMeterBounds);
    
    // Draw level meter outlines - sharp corners, no rounding
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(leftMeterBounds, 1);
    g.drawRect(rightMeterBounds, 1);
    
    // Draw input level meters
    if (audioInputManager && isProcessingActive)
    {
        float leftLevel = audioInputManager->getInputLevel(0);
        float rightLevel = audioInputManager->getInputLevel(1);
        
        // Convert to dB and normalize to 0-1 range (-60dB to 0dB)
        float leftDb = juce::Decibels::gainToDecibels(leftLevel, -60.0f);
        float rightDb = juce::Decibels::gainToDecibels(rightLevel, -60.0f);
        
        float leftNormalized = juce::jmap(leftDb, -60.0f, 0.0f, 0.0f, 1.0f);
        float rightNormalized = juce::jmap(rightDb, -60.0f, 0.0f, 0.0f, 1.0f);
        
        // Clamp values to 0-1 range
        leftNormalized = juce::jlimit(0.0f, 1.0f, leftNormalized);
        rightNormalized = juce::jlimit(0.0f, 1.0f, rightNormalized);
        
        // Draw left level meter with gradient colors
        if (leftNormalized > 0.0f)
        {
            auto leftFillArea = leftMeterBounds.reduced(1);
            int fillHeight = (int)(leftFillArea.getHeight() * leftNormalized);
            auto fillRect = leftFillArea.removeFromBottom(fillHeight);
            
            // Use gradient colors: green -> yellow -> red
            if (leftNormalized > 0.85f)
                g.setColour(juce::Colours::red);
            else if (leftNormalized > 0.7f)
                g.setColour(juce::Colours::orange);
            else if (leftNormalized > 0.5f)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
            
            g.fillRect(fillRect);
        }
        
        // Draw right level meter with gradient colors
        if (rightNormalized > 0.0f)
        {
            auto rightFillArea = rightMeterBounds.reduced(1);
            int fillHeight = (int)(rightFillArea.getHeight() * rightNormalized);
            auto fillRect = rightFillArea.removeFromBottom(fillHeight);
            
            // Use gradient colors: green -> yellow -> red
            if (rightNormalized > 0.85f)
                g.setColour(juce::Colours::red);
            else if (rightNormalized > 0.7f)
                g.setColour(juce::Colours::orange);
            else if (rightNormalized > 0.5f)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
            
            g.fillRect(fillRect);
        }
    }
    
    // Draw scale marks on level meters (optional dB markers)
    g.setColour(juce::Colour(0xff666666));
    if (!leftMeterBounds.isEmpty() && !rightMeterBounds.isEmpty())
    {
        // Draw dB scale marks at -60, -40, -20, -10, -5, 0 dB
        std::vector<float> dbMarks = {-60.0f, -40.0f, -20.0f, -10.0f, -5.0f, 0.0f};
        
        for (float db : dbMarks)
        {
            float normalizedPos = juce::jmap(db, -60.0f, 0.0f, 1.0f, 0.0f); // Inverted for bottom-up
            int yPos = leftMeterBounds.getY() + (int)(leftMeterBounds.getHeight() * normalizedPos);
            
            // Draw small tick marks
            g.drawLine(leftMeterBounds.getRight() + 2, yPos, leftMeterBounds.getRight() + 6, yPos);
            g.drawLine(rightMeterBounds.getRight() + 2, yPos, rightMeterBounds.getRight() + 6, yPos);
        }
    }
    
    // Plugin chain area border removed for cleaner look
}

void MainComponent::resized()
{
    setupLayout();
}

void MainComponent::setupLayout()
{
    auto area = getLocalBounds();
    
    // Header area - compact horizontal layout for device selections
    auto headerArea = area.removeFromTop(90);
    
    // Device selection area (labels + dropdowns + status indicators)
    auto deviceArea = headerArea.removeFromTop(55);
    deviceArea.removeFromTop(5); // Add some top padding
    
    // Split device area horizontally for input and output
    auto inputDeviceArea = deviceArea.removeFromLeft(getWidth() / 2 - 10); // Half width minus some margin
    auto outputDeviceArea = deviceArea; // Remaining space
    
    // Input device section (label above dropdown)
    auto inputLabelArea = inputDeviceArea.removeFromTop(20);
    inputDeviceLabel.setBounds(inputLabelArea.reduced(5, 0));
    
    auto inputControlArea = inputDeviceArea.removeFromTop(30);
    inputDeviceComboBox.setBounds(inputControlArea.removeFromLeft(inputControlArea.getWidth() - 25).reduced(5, 0));
    
    // Status indicator circle next to input combo box
    inputStatusIndicatorBounds = inputControlArea.reduced(5);
    inputStatusIndicatorBounds = inputStatusIndicatorBounds.withSizeKeepingCentre(10, 10);
    
    // Output device section (label above dropdown)
    auto outputLabelArea = outputDeviceArea.removeFromTop(20);
    outputDeviceLabel.setBounds(outputLabelArea.reduced(5, 0));
    
    auto outputControlArea = outputDeviceArea.removeFromTop(30);
    outputDeviceComboBox.setBounds(outputControlArea.removeFromLeft(outputControlArea.getWidth() - 25).reduced(5, 0));
    
    // Status indicator circle next to output combo box
    outputStatusIndicatorBounds = outputControlArea.reduced(5);
    outputStatusIndicatorBounds = outputStatusIndicatorBounds.withSizeKeepingCentre(10, 10);
    
    // CTA Button row - centered and prominent
    auto buttonRow = headerArea.removeFromTop(35);
    buttonRow.removeFromTop(5); // Add some spacing
    
    // Center the CTA button
    auto buttonWidth = 200;
    auto buttonArea = buttonRow.withSizeKeepingCentre(buttonWidth, 25);
    processingToggleButton.setBounds(buttonArea);
    
    // Main content area with level meters on the right
    auto contentArea = area.removeFromBottom(area.getHeight() - 10);
    contentArea = contentArea.removeFromLeft(contentArea.getWidth() - 15);

    auto levelMeterArea = contentArea.removeFromRight(80); // Reserve space for level meters
    
    // Plugin chain area (remaining space)
    pluginChainComponent->setBounds(contentArea);
    
    // Center the level meters in the available space
    auto meterWidth = 25; // Width of each meter
    auto meterSpacing = 10; // Space between meters
    auto totalMeterWidth = (meterWidth * 2) + meterSpacing; // Total width needed for both meters
    auto centerOffset = (levelMeterArea.getWidth() - totalMeterWidth) / 2; // Center offset
    
    // Create centered meter area
    auto centeredMeterArea = levelMeterArea.withTrimmedLeft(centerOffset).withWidth(totalMeterWidth);
    
    // Level meter labels (top part of centered area)
    auto labelArea = centeredMeterArea.removeFromTop(20);
    auto leftLabelArea = labelArea.removeFromLeft(meterWidth + meterSpacing/2);
    auto rightLabelArea = labelArea.removeFromLeft(meterWidth + meterSpacing/2);
    
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
void MainComponent::timerCallback()
{
    // Update level meter labels (just show L and R, the visual meters show the levels)
    leftLevelLabel.setText("L", juce::dontSendNotification);
    rightLevelLabel.setText("R", juce::dontSendNotification);
    
    // Repaint to update level meters and status indicator
    repaint();
}

// Status is now shown via visual indicator circle

void MainComponent::updateInputDeviceList()
{
    inputDeviceComboBox.clear();
    outputDeviceComboBox.clear();
    
    if (audioInputManager)
    {
        // Populate input devices
        auto inputDevices = audioInputManager->getAvailableInputDevices();
        
        for (int i = 0; i < inputDevices.size(); ++i)
        {
            inputDeviceComboBox.addItem(inputDevices[i], i + 1);
        }
        
        if (inputDevices.size() > 0)
        {
            // Try to select microphone by default instead of BlackHole for testing
            int micIndex = -1;
            for (int i = 0; i < inputDevices.size(); ++i)
            {
                if (inputDevices[i].containsIgnoreCase("microphone"))
                {
                    micIndex = i;
                    break;
                }
            }
            
            if (micIndex >= 0)
            {
                inputDeviceComboBox.setSelectedItemIndex(micIndex);
                DBG("Auto-selected microphone for testing");
            }
            else
            {
                inputDeviceComboBox.setSelectedItemIndex(0);
            }
            inputDeviceChanged(); // Set the selected device
        }
        
        // Populate output devices
        auto outputDevices = audioInputManager->getAvailableOutputDevices();
        
        for (int i = 0; i < outputDevices.size(); ++i)
        {
            outputDeviceComboBox.addItem(outputDevices[i], i + 1);
        }
        
        if (outputDevices.size() > 0)
        {
            outputDeviceComboBox.setSelectedItemIndex(0);
            outputDeviceChanged(); // Set the selected device
        }
    }
}

//==============================================================================
void MainComponent::toggleProcessing()
{
    if (isProcessingActive)
    {
        // Stop processing
        // Remove ourselves as the audio callback
        if (audioInputManager)
        {
            audioInputManager->getAudioDeviceManager().removeAudioCallback(this);
            audioInputManager->stop();
        }
        
        isProcessingActive = false;
        processingToggleButton.setButtonText("Start Processing");
        processingToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff007acc)); // Blue for start
        inputDeviceComboBox.setEnabled(true);
        outputDeviceComboBox.setEnabled(true);
        
        if (audioProcessor)
            audioProcessor->stop();
        
        DBG("Audio processing stopped");
    }
    else
    {
        // Start processing
        if (!audioInputManager || !audioInputManager->hasValidInputDevice())
        {
            DBG("No input device selected");
            return;
        }
        
        if (audioInputManager->start())
        {
            // Add ourselves as the audio callback
            audioInputManager->getAudioDeviceManager().addAudioCallback(this);
            
            isProcessingActive = true;
            processingToggleButton.setButtonText("Stop Processing");
            processingToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc3300)); // Red for stop
            inputDeviceComboBox.setEnabled(false);
            outputDeviceComboBox.setEnabled(false);
            
            // Start audio processor
            if (audioProcessor)
                audioProcessor->start();
            
            DBG("Audio processing started from: " + audioInputManager->getCurrentInputDevice());
        }
        else
        {
            DBG("Failed to start audio processing");
        }
    }
}

void MainComponent::inputDeviceChanged()
{
    int selectedIndex = inputDeviceComboBox.getSelectedItemIndex();
    
    DBG("Input device selection changed - index: " + juce::String(selectedIndex));
    
    if (selectedIndex >= 0 && audioInputManager)
    {
        juce::String deviceName = inputDeviceComboBox.getItemText(selectedIndex);
        
        DBG("User selected device: '" + deviceName + "'");
        
        if (audioInputManager->setInputDevice(deviceName))
        {
            DBG("Input device changed to: " + deviceName);
        }
        else
        {
            DBG("Failed to set input device: " + deviceName);
        }
    }
}

void MainComponent::outputDeviceChanged()
{
    int selectedIndex = outputDeviceComboBox.getSelectedItemIndex();
    
    DBG("Output device selection changed - index: " + juce::String(selectedIndex));
    
    if (selectedIndex >= 0 && audioInputManager)
    {
        juce::String deviceName = outputDeviceComboBox.getItemText(selectedIndex);
        
        DBG("User selected output device: '" + deviceName + "'");
        
        if (audioInputManager->setOutputDevice(deviceName))
        {
            DBG("Output device changed to: " + deviceName);
        }
        else
        {
            DBG("Failed to set output device: " + deviceName);
        }
    }
} 