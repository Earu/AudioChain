#include "PluginChainComponent.h"

//==============================================================================
PluginChainComponent::PluginChainComponent(PluginHost &pluginHost_) : pluginHost(pluginHost_) {
    // Initialize scrollable plugin chain
    chainContainer = std::make_unique<PluginChainContainer>(*this);
    chainViewport.setViewedComponent(chainContainer.get(), false);
    chainViewport.setScrollBarsShown(true, false); // Vertical scrollbar only
    addAndMakeVisible(chainViewport);

    // Initialize level meters
    // Level meters are now handled by MainComponent
    // for (int i = 0; i < 2; ++i)
    // {
    //     levelMeters[i] = std::make_unique<LevelMeter>();
    //     addAndMakeVisible(*levelMeters[i]);
    // }

    // Setup controls with dark theme
    addAndMakeVisible(addPluginButton);
    addAndMakeVisible(clearAllButton);
    addAndMakeVisible(chainLabel);

    addPluginButton.setButtonText("Add Plugin");
    clearAllButton.setButtonText("Clear All");
    chainLabel.setText("Plugin Chain", juce::dontSendNotification);
    chainLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    chainLabel.setJustificationType(juce::Justification::centred);

    // Apply dark theme to buttons
    addPluginButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    addPluginButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    addPluginButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addPluginButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    clearAllButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    clearAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    clearAllButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    // Apply dark theme to label
    chainLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Button callbacks
    addPluginButton.onClick = [this] { showPluginBrowser(); };
    clearAllButton.onClick = [this] { pluginHost.clearAllPlugins(); };

    // Setup plugin browser (initially hidden)
    pluginBrowser = std::make_unique<PluginBrowser>(pluginHost);
    addChildComponent(*pluginBrowser);

    // Setup plugin host callbacks
    pluginHost.onPluginChainChanged = [this] { onPluginChainChanged(); };
    pluginHost.onPluginError = [this](int index, const juce::String &error) { onPluginError(index, error); };
    pluginHost.onPluginScanComplete = [this] { onPluginScanComplete(); };

    // Start timer for updates
    startTimer(50); // Update every 50ms

    // Initial refresh
    refreshPluginChain();
}

PluginChainComponent::~PluginChainComponent() {
    stopTimer();

    // Close all plugin editors
    for (int i = 0; i < pluginSlots.size(); ++i) {
        closePluginEditor(i);
    }
}

//==============================================================================
void PluginChainComponent::paint(juce::Graphics &g) {
    // Background is handled by the main app
    // Connecting lines are now handled by the PluginChainContainer
}

void PluginChainComponent::resized() {
    auto area = getLocalBounds();

    // Control area at top
    controlArea = area.removeFromTop(40);
    chainLabel.setBounds(controlArea.removeFromLeft(120));
    addPluginButton.setBounds(controlArea.removeFromLeft(100).reduced(2));
    clearAllButton.setBounds(controlArea.removeFromLeft(80).reduced(2));

    // Chain area (remaining space) - now uses viewport
    chainArea = area.reduced(10);
    chainViewport.setBounds(chainArea);

    // Update the container size and layout
    if (chainContainer) {
        chainContainer->updateSlots();
    }

    // Plugin browser (full component)
    if (pluginBrowser) {
        pluginBrowser->setBounds(getLocalBounds());
    }
}

void PluginChainComponent::timerCallback() {
    // Level meters are now handled by MainComponent
    // Update level meters (placeholder - would need actual audio level data)
    // In a real implementation, you'd get level data from the audio processor
    // for (int i = 0; i < 2; ++i)
    // {
    //     float level = 0.0f; // Replace with actual level data
    //     levelMeters[i]->setLevel(level);
    // }
}

//==============================================================================
void PluginChainComponent::refreshPluginChain() {
    int numPlugins = pluginHost.getNumPlugins();
    int slotsNeeded = numPlugins + 1; // Loaded plugins + one empty slot

    // Add slots if we need more
    while (pluginSlots.size() < slotsNeeded) {
        int newIndex = pluginSlots.size();
        auto newSlot = std::make_unique<PluginSlot>(newIndex, pluginHost, *this);
        chainContainer->addAndMakeVisible(newSlot.get());
        pluginSlots.add(newSlot.release());
    }

    // Remove excess slots if we have too many (keep only one empty slot)
    while (pluginSlots.size() > slotsNeeded) {
        pluginSlots.removeLast();
    }

    // Update existing slots with current chain state
    for (int i = 0; i < pluginSlots.size(); ++i) {
        if (i < numPlugins) {
            auto info = pluginHost.getPluginInfo(i);
            pluginSlots[i]->setPluginInfo(info);
        } else {
            pluginSlots[i]->clearPlugin();
        }
    }

    // Update container layout
    if (chainContainer) {
        chainContainer->updateSlots();
    }
}

void PluginChainComponent::showPluginBrowser() {
    if (pluginBrowser) {
        pluginBrowser->refreshPluginList();
        pluginBrowser->setVisible(true);
        pluginBrowser->toFront(true);
    }
}

void PluginChainComponent::hidePluginBrowser() {
    if (pluginBrowser) {
        pluginBrowser->setVisible(false);
    }
}

void PluginChainComponent::openPluginEditor(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= pluginSlots.size())
        return;

    // Close existing editor for this slot
    closePluginEditor(slotIndex);

    auto *editor = pluginHost.createEditorForPlugin(slotIndex);
    if (editor) {
        auto info = pluginHost.getPluginInfo(slotIndex);
        juce::String windowTitle = info.name + " - " + info.manufacturer;

        auto *window = new PluginEditorWindow(windowTitle, slotIndex, *this);
        window->setContentOwned(editor, true);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);

        editorWindows.add(window);
    }
}

void PluginChainComponent::closePluginEditor(int slotIndex) {
    // Find and close the editor window for this slot
    for (int i = editorWindows.size() - 1; i >= 0; --i) {
        auto *window = editorWindows[i];
        if (window->getSlotIndex() == slotIndex) {
            // Close the PluginHost editor first
            pluginHost.closeEditorForPlugin(slotIndex);

            // Remove the window (this will delete it safely)
            editorWindows.remove(i);
            break;
        }
    }
}

void PluginChainComponent::onEditorWindowClosed(int slotIndex) {
    // Called when user clicks the close button on a plugin editor window
    // This ensures proper cleanup when the window is closed by the user
    juce::MessageManager::callAsync([this, slotIndex]() { closePluginEditor(slotIndex); });
}

void PluginChainComponent::onPluginChainChanged() { refreshPluginChain(); }

void PluginChainComponent::onPluginError(int pluginIndex, const juce::String &error) {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Plugin Error",
                                           "Plugin " + juce::String(pluginIndex) + ": " + error, "OK");
}

void PluginChainComponent::onPluginScanComplete() {
    DBG("Plugin scan completed - refreshing UI");

    // Refresh the plugin browser if it's visible
    if (pluginBrowser && pluginBrowser->isVisible()) {
        pluginBrowser->onScanComplete();
    }
}

void PluginChainComponent::handleDraggedPlugin(int fromSlot, int toSlot) {
    // Convert UI slot indices to actual plugin chain indices
    // Only move if both slots are valid and we're moving to a different slot
    if (fromSlot >= 0 && fromSlot < pluginSlots.size() && toSlot >= 0 && toSlot < pluginSlots.size() && fromSlot != toSlot) {
        // Check if source slot has a plugin
        if (fromSlot < pluginHost.getNumPlugins()) {
            // If target slot is beyond current plugins, we're appending
            if (toSlot >= pluginHost.getNumPlugins()) {
                // Move to end of chain
                pluginHost.movePlugin(fromSlot, pluginHost.getNumPlugins() - 1);
            } else {
                // Move within existing chain
                pluginHost.movePlugin(fromSlot, toSlot);
            }
        }
    }
}

//==============================================================================
// PluginSlot Implementation
//==============================================================================
PluginChainComponent::PluginSlot::PluginSlot(int index, PluginHost &host, PluginChainComponent &parent)
    : slotIndex(index), pluginHost(host), parentComponent(parent) {
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(manufacturerLabel);
    addAndMakeVisible(editButton);
    addAndMakeVisible(removeButton);

    // Setup labels for horizontal layout
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::Font("Arial Black", 20.0f, juce::Font::bold)); // Increased from 16pt to 20pt
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel.setInterceptsMouseClicks(false, false); // Allow mouse events to pass through

    manufacturerLabel.setJustificationType(juce::Justification::centredLeft);
    manufacturerLabel.setFont(juce::Font("Consolas", 12.0f, juce::Font::plain));
    manufacturerLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    manufacturerLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    manufacturerLabel.setInterceptsMouseClicks(false, false); // Allow mouse events to pass through

    // Setup buttons with modern styling
    editButton.setButtonText("Edit");
    removeButton.setButtonText("Remove");

    // Apply dark theme to buttons
    editButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    editButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    editButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    editButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    removeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff404040));
    removeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    removeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    editButton.addListener(this);
    removeButton.addListener(this);

    // Generate slot color
    slotColour = juce::Colour::fromHSL(index * 0.123f, 0.6f, 0.7f, 1.0f);

    // Initially empty
    clearPlugin();
}

PluginChainComponent::PluginSlot::~PluginSlot() {
    editButton.removeListener(this);
    removeButton.removeListener(this);
}

void PluginChainComponent::PluginSlot::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds();

    // Draw drag over highlight first (behind everything else)
    if (isDragOver) {
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.3f));
        g.fillRect(bounds);

        g.setColour(juce::Colour(0xff00d4ff));
        g.drawRect(bounds, 3);
    }

    if (hasPlugin()) {
        // Store original bounds for shadow
        auto originalBounds = bounds;

        // Draw enhanced plugin background
        drawPluginBackground(g, bounds);

        // Draw status indicator on the left
        drawStatusIndicator(g, bounds);

        // Add subtle shadow effect when not bypassed - use original bounds
        if (!isBypassed) {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.drawRect(originalBounds.expanded(1), 1);
        }
    } else {
        // Draw empty slot with hover feedback
        juce::Colour bgColor = isEmptySlotHovered ? juce::Colour(0xff404040) : juce::Colour(0xff2d2d2d);
        g.setColour(bgColor);
        g.fillRect(bounds);

        // Draw dashed border for empty slots
        juce::Colour borderColor = isEmptySlotHovered ? juce::Colour(0xff606060) : juce::Colour(0xff404040);
        g.setColour(borderColor);
        float dashLengths[] = {4.0f, 4.0f};
        auto borderBounds = bounds.toFloat();
        g.drawDashedLine(
            juce::Line<float>(borderBounds.getX(), borderBounds.getY(), borderBounds.getRight(), borderBounds.getY()),
            dashLengths, 2);
        g.drawDashedLine(juce::Line<float>(borderBounds.getRight(), borderBounds.getY(), borderBounds.getRight(),
                                           borderBounds.getBottom()),
                         dashLengths, 2);
        g.drawDashedLine(juce::Line<float>(borderBounds.getRight(), borderBounds.getBottom(), borderBounds.getX(),
                                           borderBounds.getBottom()),
                         dashLengths, 2);
        g.drawDashedLine(
            juce::Line<float>(borderBounds.getX(), borderBounds.getBottom(), borderBounds.getX(), borderBounds.getY()),
            dashLengths, 2);

        // Calculate text height to adjust icon position
        juce::Font textFont(14.0f);
        int textHeight = textFont.getHeight();
        int textMargin = 4;

        // Draw "+" icon for empty slots - centered in the space above the text
        juce::Colour iconColor = isEmptySlotHovered ? juce::Colour(0xff888888) : juce::Colour(0xff666666);
        g.setColour(iconColor);
        auto centerX = bounds.getCentreX();
        auto iconCenterY = bounds.getCentreY() - (textHeight + textMargin) / 2; // Shift up to account for text
        auto iconSize = 20;
        g.drawLine(centerX - iconSize / 2, iconCenterY, centerX + iconSize / 2, iconCenterY, 2.0f);
        g.drawLine(centerX, iconCenterY - iconSize / 2, centerX, iconCenterY + iconSize / 2, 2.0f);

        // Draw "Empty" text
        g.setColour(iconColor);
        g.drawText("Empty", bounds.reduced(4), juce::Justification::centredBottom);
    }
}

void PluginChainComponent::PluginSlot::resized() {
    auto bounds = getLocalBounds().reduced(4);

    if (hasPlugin()) {
        // Horizontal layout: [Toggle] [Text Area] [Buttons]

        // Status indicator (toggle) on the left
        statusIndicatorBounds = juce::Rectangle<int>(bounds.getX() + 10, bounds.getCentreY() - 8, 16, 16);
        auto toggleArea = bounds.removeFromLeft(40); // Space for toggle + margin

        // Buttons on the right
        auto buttonWidth = 60;
        auto buttonSpacing = 4;
        auto totalButtonWidth = (buttonWidth * 2) + buttonSpacing + 8; // Two buttons + spacing + margins
        auto buttonArea = bounds.removeFromRight(totalButtonWidth);

        // Split button area horizontally
        editButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        buttonArea.removeFromLeft(buttonSpacing);
        removeButton.setBounds(buttonArea.reduced(2));

        // Text area gets the remaining middle space
        auto textArea = bounds.reduced(8, 4); // Some padding

        // Split text area vertically: plugin name on top, manufacturer below
        auto nameHeight = textArea.getHeight() * 0.6f; // 60% for plugin name
        nameLabel.setBounds(textArea.removeFromTop(nameHeight));
        manufacturerLabel.setBounds(textArea); // Rest for manufacturer
    } else {
        // Empty slot - just center everything
        statusIndicatorBounds = juce::Rectangle<int>();
        nameLabel.setBounds(bounds);
        manufacturerLabel.setBounds(juce::Rectangle<int>());
        editButton.setBounds(juce::Rectangle<int>());
        removeButton.setBounds(juce::Rectangle<int>());
    }
}

void PluginChainComponent::PluginSlot::buttonClicked(juce::Button *button) {
    if (!hasPlugin())
        return;

    if (button == &editButton) {
        parentComponent.openPluginEditor(slotIndex);
    } else if (button == &removeButton) {
        pluginHost.unloadPlugin(slotIndex);
    }
}

void PluginChainComponent::PluginSlot::mouseDown(const juce::MouseEvent &event) {
    if (!hasPlugin()) {
        // Clicking on empty slot opens the plugin browser
        parentComponent.showPluginBrowser();
        return;
    }

    // Store the mouse down position for later use in mouseDrag
    mouseDownPosition = event.getPosition();
}

void PluginChainComponent::PluginSlot::mouseUp(const juce::MouseEvent &event) {
    if (!hasPlugin())
        return;

    // Only process click if we haven't moved much (not a drag)
    if (event.getDistanceFromDragStart() < 5) {
        // Check if click is within the status indicator bounds
        if (statusIndicatorBounds.contains(event.getPosition())) {
            bool currentlyBypassed = pluginHost.isPluginBypassed(slotIndex);
            pluginHost.bypassPlugin(slotIndex, !currentlyBypassed);
            updateBypassState();
        }
    }
}

void PluginChainComponent::PluginSlot::mouseDrag(const juce::MouseEvent &event) {
    if (!hasPlugin())
        return;

    // Don't start drag if we're over the status indicator
    if (statusIndicatorBounds.contains(mouseDownPosition))
        return;

    // Don't start drag if we're actually over a button (not just in the button area)
    // Check if the buttons are visible and if we're actually over them
    if (editButton.isVisible() && editButton.getBounds().contains(mouseDownPosition))
        return;
    if (removeButton.isVisible() && removeButton.getBounds().contains(mouseDownPosition))
        return;

    // Start drag operation if we've moved far enough
    if (event.getDistanceFromDragStart() > 15) // Increased threshold to avoid accidental drags
    {
        // Create drag description with plugin slot index
        juce::var dragData;
        dragData = slotIndex;

        // Create a visual representation of the plugin for dragging
        auto dragImage = createComponentSnapshot(getLocalBounds());

        // Start the drag operation - the parentComponent is a DragAndDropContainer
        parentComponent.startDragging(dragData, this, dragImage, true);
    }
}

void PluginChainComponent::PluginSlot::mouseMove(const juce::MouseEvent &event) {
    if (!hasPlugin()) {
        // Handle empty slot hover state
        bool wasEmptyHovered = isEmptySlotHovered;
        isEmptySlotHovered = true;

        // Show pointer cursor for empty slots to indicate they're clickable
        setMouseCursor(juce::MouseCursor::PointingHandCursor);

        // Repaint if hover state changed
        if (wasEmptyHovered != isEmptySlotHovered) {
            repaint();
        }
        return;
    }

    bool wasHovered = isStatusIndicatorHovered;
    isStatusIndicatorHovered = statusIndicatorBounds.contains(event.getPosition());

    // Determine cursor based on what we're hovering over
    if (isStatusIndicatorHovered) {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    } else if ((editButton.isVisible() && editButton.getBounds().contains(event.getPosition())) ||
               (removeButton.isVisible() && removeButton.getBounds().contains(event.getPosition()))) {
        setMouseCursor(juce::MouseCursor::PointingHandCursor); // Buttons are clickable
    } else {
        // Over the main plugin area - show drag cursor to indicate it's draggable
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    // Only repaint if hover state changed
    if (wasHovered != isStatusIndicatorHovered) {
        repaint();
    }
}

void PluginChainComponent::PluginSlot::mouseExit(const juce::MouseEvent &event) {
    // Always reset cursor when exiting
    setMouseCursor(juce::MouseCursor::NormalCursor);

    bool needsRepaint = false;

    if (isStatusIndicatorHovered) {
        isStatusIndicatorHovered = false;
        needsRepaint = true;
    }

    if (isEmptySlotHovered) {
        isEmptySlotHovered = false;
        needsRepaint = true;
    }

    if (needsRepaint) {
        repaint();
    }
}

//==============================================================================
// Drag and Drop Target Implementation
//==============================================================================
bool PluginChainComponent::PluginSlot::isInterestedInDragSource(const SourceDetails &dragSourceDetails) {
    // We're interested in plugin slot drags (identified by integer slot index)
    return dragSourceDetails.description.isInt();
}

void PluginChainComponent::PluginSlot::itemDragEnter(const SourceDetails &dragSourceDetails) {
    isDragOver = true;
    repaint();
}

void PluginChainComponent::PluginSlot::itemDragMove(const SourceDetails &dragSourceDetails) {
    // Visual feedback is handled by isDragOver flag
}

void PluginChainComponent::PluginSlot::itemDragExit(const SourceDetails &dragSourceDetails) {
    isDragOver = false;
    repaint();
}

void PluginChainComponent::PluginSlot::itemDropped(const SourceDetails &dragSourceDetails) {
    isDragOver = false;

    if (dragSourceDetails.description.isInt()) {
        int fromSlotIndex = dragSourceDetails.description;
        int toSlotIndex = slotIndex;

        if (fromSlotIndex != toSlotIndex) {
            parentComponent.handleDraggedPlugin(fromSlotIndex, toSlotIndex);
        }
    }

    repaint();
}

void PluginChainComponent::PluginSlot::setPluginInfo(const PluginHost::PluginInfo &info) {
    pluginInfo = info;

    // Set text with uppercase plugin name for impact
    nameLabel.setText(info.name.toUpperCase(), juce::dontSendNotification);
    manufacturerLabel.setText(info.manufacturer, juce::dontSendNotification);

    // Generate visual theme
    generatePluginTheme();

    updateBypassState();

    // Show controls
    editButton.setVisible(true);
    removeButton.setVisible(true);

    repaint();
    resized();
}

void PluginChainComponent::PluginSlot::clearPlugin() {
    pluginInfo = {};

    nameLabel.setText("", juce::dontSendNotification);
    manufacturerLabel.setText("", juce::dontSendNotification);

    // Reset visual elements
    primaryColour = juce::Colours::grey;
    secondaryColour = juce::Colours::darkgrey;
    accentColour = juce::Colours::lightgrey;

    // Hide controls
    editButton.setVisible(false);
    removeButton.setVisible(false);

    repaint();
}

void PluginChainComponent::PluginSlot::updateBypassState() {
    if (hasPlugin()) {
        isBypassed = pluginHost.isPluginBypassed(slotIndex);
        repaint(); // Repaint to update the status indicator appearance
    }
}

void PluginChainComponent::PluginSlot::generatePluginTheme() {
    if (!hasPlugin())
        return;

    // Generate colors based on plugin name and manufacturer using hash
    primaryColour = getHashBasedColour(pluginInfo.manufacturer, 0.7f, 0.6f);
    secondaryColour = getHashBasedColour(pluginInfo.name, 0.6f, 0.4f);

    // Create accent color by blending primary and secondary
    accentColour = primaryColour.interpolatedWith(secondaryColour, 0.5f).withBrightness(0.8f);

    // Ensure colors work well with dark theme
    primaryColour = primaryColour.withSaturation(0.7f).withBrightness(0.6f);
    secondaryColour = secondaryColour.withSaturation(0.6f).withBrightness(0.4f);
}

void PluginChainComponent::PluginSlot::drawPluginBackground(juce::Graphics &g, const juce::Rectangle<int> &bounds) {
    // Draw procedural pattern that fills the entire slot
    drawProceduralPattern(g, bounds);

    // Draw darker border for better text readability
    juce::Colour borderColor = isBypassed ? juce::Colour(0xff333333) : juce::Colour(0xff555555);
    g.setColour(borderColor.withAlpha(isBypassed ? 0.4f : 0.7f));
    g.drawRect(bounds, isBypassed ? 1 : 2);
}

void PluginChainComponent::PluginSlot::drawProceduralPattern(juce::Graphics &g, const juce::Rectangle<int> &bounds) {
    // Fill entire slot with black background
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);

    // Create clean topographic pattern with white lines
    auto hash = pluginInfo.name.hashCode();
    juce::Random random(hash);

    // Create unique elevation center based on plugin hash - keep well within bounds
    float centerVariationX = ((hash % 100) / 100.0f) * 0.4f + 0.3f; // 0.3 to 0.7 (more centered)
    float centerVariationY = (((hash / 100) % 100) / 100.0f) * 0.4f + 0.3f; // 0.3 to 0.7 (more centered)
    juce::Point<float> center(
        bounds.getX() + bounds.getWidth() * centerVariationX,
        bounds.getY() + bounds.getHeight() * centerVariationY
    );

    // Draw darker contour lines for better text readability
    juce::Colour lineColor = isBypassed ? juce::Colour(0xff333333) : juce::Colour(0xff666666);
    g.setColour(lineColor.withAlpha(isBypassed ? 0.3f : 0.6f));

    // Vary contour density and size based on plugin hash for uniqueness
    int numContours = 15 + (hash % 10); // 15-24 contours for variation
    float radiusMultiplier = 1.0f + ((hash % 50) / 100.0f); // 1.0 to 1.5 for better coverage
    // Use diagonal distance to ensure we can fill corners
    float maxRadius = std::sqrt(bounds.getWidth() * bounds.getWidth() + bounds.getHeight() * bounds.getHeight()) * radiusMultiplier;

    for (int contour = 1; contour < numContours; ++contour) {
        float elevation = (float)contour / numContours;
        float contourDistance = elevation * maxRadius;

        // Create organic contour shape by collecting all valid points first
        int samples = 64;
        std::vector<juce::Point<float>> validPoints;

        for (int sample = 0; sample <= samples; ++sample) {
            float angle = (float)sample / samples * juce::MathConstants<float>::twoPi;

            // Add unique organic variation based on plugin hash
            float freq1 = 3 + (hash % 5); // 3-7
            float freq2 = 6 + ((hash / 10) % 6); // 6-11
            float amp1 = 0.08f + ((hash % 20) / 200.0f); // 0.08-0.18
            float amp2 = 0.03f + ((hash % 15) / 300.0f); // 0.03-0.08

            float variation =
                std::sin(angle * freq1 + hash * 0.01f) * contourDistance * amp1 +
                std::sin(angle * freq2 + hash * 0.02f) * contourDistance * amp2;

            float radius = contourDistance + variation;

            float x = center.x + std::cos(angle) * radius;
            float y = center.y + std::sin(angle) * radius;

            // Clip points to bounds instead of excluding them entirely
            x = juce::jlimit((float)bounds.getX(), (float)bounds.getRight(), x);
            y = juce::jlimit((float)bounds.getY(), (float)bounds.getBottom(), y);

            validPoints.push_back({x, y});
        }

        // Draw the clipped contour
        if (validPoints.size() > 3) {
            juce::Path contourPath;
            contourPath.startNewSubPath(validPoints[0]);

            for (size_t i = 1; i < validPoints.size(); ++i) {
                contourPath.lineTo(validPoints[i]);
            }

            // Check if we should close the path by looking at the original unclipped points
            float originalX1 = center.x + std::cos(0) * contourDistance;
            float originalY1 = center.y + std::sin(0) * contourDistance;
            float originalX2 = center.x + std::cos(juce::MathConstants<float>::twoPi) * contourDistance;
            float originalY2 = center.y + std::sin(juce::MathConstants<float>::twoPi) * contourDistance;

            // If both start and end points would be within bounds (unclipped), close the path
            bool startInBounds = (originalX1 >= bounds.getX() && originalX1 <= bounds.getRight() &&
                                originalY1 >= bounds.getY() && originalY1 <= bounds.getBottom());
            bool endInBounds = (originalX2 >= bounds.getX() && originalX2 <= bounds.getRight() &&
                              originalY2 >= bounds.getY() && originalY2 <= bounds.getBottom());

            if (startInBounds && endInBounds) {
                contourPath.closeSubPath();
            }

            // Use consistent stroke width for clean look
            float strokeWidth = isBypassed ? 0.8f : 1.2f;
            g.strokePath(contourPath, juce::PathStrokeType(strokeWidth));
        }
    }
}

void PluginChainComponent::PluginSlot::drawPluginIcon(juce::Graphics &g, const juce::Rectangle<int> &iconArea) {
    if (iconArea.isEmpty())
        return;

    // Use white color for visibility against black background
    juce::Colour iconColor = isBypassed ? juce::Colour(0xff666666) : juce::Colours::white;
    g.setColour(iconColor);

    if (pluginInfo.isInstrument) {
        // Draw musical note icon for instruments
        juce::Path notePath;
        auto center = iconArea.getCentre();
        auto size = juce::jmin(iconArea.getWidth(), iconArea.getHeight()) * 0.6f;

        // Musical note shape
        notePath.addEllipse(center.x - size * 0.2f, center.y, size * 0.4f, size * 0.3f);
        notePath.addRectangle(center.x + size * 0.15f, center.y - size * 0.4f, size * 0.1f, size * 0.7f);

        g.fillPath(notePath);
    } else {
        // Draw waveform icon for effects
        juce::Path wavePath;
        auto startX = iconArea.getX() + iconArea.getWidth() * 0.1f;
        auto endX = iconArea.getRight() - iconArea.getWidth() * 0.1f;
        auto centerY = iconArea.getCentreY();
        auto amplitude = iconArea.getHeight() * 0.3f;

        wavePath.startNewSubPath(startX, centerY);
        for (float x = startX; x <= endX; x += 2.0f) {
            float progress = (x - startX) / (endX - startX);
            float y = centerY + amplitude * std::sin(progress * 6.28f * 2); // 2 cycles
            wavePath.lineTo(x, y);
        }

        g.strokePath(wavePath, juce::PathStrokeType(3.0f)); // Thicker stroke for visibility
    }
}

juce::Colour PluginChainComponent::PluginSlot::getHashBasedColour(const juce::String &text, float saturation,
                                                                  float brightness) {
    // Create a hash from the text
    auto hash = text.hashCode();

    // Use hash to generate hue (0-360 degrees)
    float hue = (hash % 360) / 360.0f;

    return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

void PluginChainComponent::PluginSlot::drawStatusIndicator(juce::Graphics &g, const juce::Rectangle<int> &bounds) {
    if (statusIndicatorBounds.isEmpty())
        return;

    auto indicatorBounds = statusIndicatorBounds.toFloat();

    // Enhanced glow effect when hovered
    if (isStatusIndicatorHovered) {
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.3f));
        g.fillEllipse(indicatorBounds.expanded(6));
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.5f));
        g.fillEllipse(indicatorBounds.expanded(3));
    }

    // Draw glow effect when active
    if (!isBypassed) {
        g.setColour(juce::Colour(0xff00ff88).withAlpha(0.2f));
        g.fillEllipse(indicatorBounds.expanded(4));
        g.setColour(juce::Colour(0xff00ff88).withAlpha(0.4f));
        g.fillEllipse(indicatorBounds.expanded(2));
    }

    // Main status indicator circle
    juce::Colour mainColour = isBypassed ? juce::Colour(0xff444444) : juce::Colour(0xff00ff88);
    if (isStatusIndicatorHovered)
        mainColour = mainColour.brighter(0.3f);

    g.setColour(mainColour);
    g.fillEllipse(indicatorBounds);

    // Draw inner pattern for active state
    if (!isBypassed) {
        // Draw a bright center dot
        auto centerBounds = indicatorBounds.reduced(3);
        g.setColour(juce::Colour(0xffffffff).withAlpha(isStatusIndicatorHovered ? 1.0f : 0.9f));
        g.fillEllipse(centerBounds);

        // Add pulsing ring effect
        g.setColour(juce::Colour(0xff00ff88).withAlpha(0.8f));
        g.drawEllipse(indicatorBounds.reduced(1), isStatusIndicatorHovered ? 2.0f : 1.5f);
    } else {
        // Draw X pattern for bypassed state
        g.setColour(isStatusIndicatorHovered ? juce::Colour(0xffaaaaaa) : juce::Colour(0xff888888));
        auto center = indicatorBounds.getCentre();
        auto size = indicatorBounds.getWidth() * 0.3f;
        auto lineWidth = isStatusIndicatorHovered ? 2.0f : 1.5f;
        g.drawLine(center.x - size, center.y - size, center.x + size, center.y + size, lineWidth);
        g.drawLine(center.x - size, center.y + size, center.x + size, center.y - size, lineWidth);
    }

    // Outer border with hover effect
    g.setColour(isStatusIndicatorHovered ? juce::Colour(0xff00d4ff) : juce::Colour(0xff222222));
    g.drawEllipse(indicatorBounds, isStatusIndicatorHovered ? 1.5f : 1.0f);
}

void PluginChainComponent::PluginSlot::drawChromaticAberrationText(juce::Graphics &g, const juce::Rectangle<int> &bounds) {
    // This method is no longer used - text is now drawn by the labels themselves
    // The new horizontal layout uses standard JUCE labels positioned in resized()
}

//==============================================================================
// PluginBrowser Implementation
//==============================================================================
PluginChainComponent::PluginBrowser::PluginBrowser(PluginHost &host)
    : pluginHost(host), searchPathsModel(*this), tabs(juce::TabbedButtonBar::TabsAtTop) {
    // Setup tabs with modern styling
    addAndMakeVisible(tabs);
    tabs.setTabBarDepth(30); // Taller tabs for better proportions

    // Style the tab bar with futuristic colors and smaller text
    tabs.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff0a0a0a));
    tabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xff333333));
    tabs.setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff444444));
    tabs.setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colour(0xffaaaaaa));
    tabs.setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::white);

    // Plugin List Tab - remove redundant header
    pluginListTab.addAndMakeVisible(pluginList);
    pluginListTab.addAndMakeVisible(refreshButton);

    // Modern button styling with gradient-like effect
    refreshButton.setButtonText("REFRESH PLUGINS");
    refreshButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    refreshButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d4ff));
    refreshButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    refreshButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    refreshButton.addListener(this);

    // Modern list styling
    pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0f0f0f));
    pluginList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff333333));
    pluginList.setModel(this);
    pluginList.setMultipleSelectionEnabled(false);
    pluginList.setRowHeight(50); // Taller rows for better readability

    tabs.addTab("PLUGINS", juce::Colour(0xff1a1a1a), &pluginListTab, false);

    // Search Paths Tab - remove redundant header
    searchPathsTab.addAndMakeVisible(searchPathsList);
    searchPathsTab.addAndMakeVisible(addPathButton);
    searchPathsTab.addAndMakeVisible(removePathButton);
    searchPathsTab.addAndMakeVisible(resetToDefaultsButton);

    // Modern button styling with different accent colors
    addPathButton.setButtonText("ADD PATH");
    removePathButton.setButtonText("REMOVE PATH");
    resetToDefaultsButton.setButtonText("RESET TO DEFAULTS");

    // Green accent for add button
    addPathButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    addPathButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00ff88));
    addPathButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addPathButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    // Red accent for remove button
    removePathButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    removePathButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4444));
    removePathButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    removePathButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    // Orange accent for reset button
    resetToDefaultsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    resetToDefaultsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffaa00));
    resetToDefaultsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    resetToDefaultsButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addPathButton.addListener(this);
    removePathButton.addListener(this);
    resetToDefaultsButton.addListener(this);

    // Modern list styling
    searchPathsList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0f0f0f));
    searchPathsList.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff333333));
    searchPathsList.setModel(&searchPathsModel);
    searchPathsList.setMultipleSelectionEnabled(false);
    searchPathsList.setRowHeight(50); // Taller rows for better readability

    tabs.addTab("SEARCH PATHS", juce::Colour(0xff1a1a1a), &searchPathsTab, false);

    // Modern close button with transparent background
    addAndMakeVisible(closeButton);
    closeButton.setButtonText("X");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4444).withAlpha(0.3f));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
    closeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    closeButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    closeButton.addListener(this);
}

PluginChainComponent::PluginBrowser::~PluginBrowser() {
    refreshButton.removeListener(this);
    closeButton.removeListener(this);
    addPathButton.removeListener(this);
    removePathButton.removeListener(this);
    resetToDefaultsButton.removeListener(this);

    pluginList.setModel(nullptr);
    searchPathsList.setModel(nullptr);
}

void PluginChainComponent::PluginBrowser::paint(juce::Graphics &g) {
    // Black background matching the main app exactly
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Draw button bar separators for active tab
    auto tabBounds = tabs.getBounds().toFloat();
    auto contentBounds = tabBounds;
    contentBounds.removeFromTop(tabs.getTabBarDepth());

    // Draw separator line above button bar
    auto buttonBarTop = contentBounds.getBottom() - 50;
    g.setColour(juce::Colour(0xff333333));
    g.drawHorizontalLine(buttonBarTop, contentBounds.getX(), contentBounds.getRight());

    // Add subtle highlight above the separator
    g.setColour(juce::Colour(0xff555555).withAlpha(0.5f));
    g.drawHorizontalLine(buttonBarTop - 1, contentBounds.getX(), contentBounds.getRight());
}

void PluginChainComponent::PluginBrowser::resized() {
    auto bounds = getLocalBounds().reduced(8);

    // Main tabs area
    tabs.setBounds(bounds);

    // Position close button in the top-right corner of the tab bar area
    auto tabBarBounds = tabs.getBounds();
    closeButton.setBounds(tabBarBounds.getRight() - 32, tabBarBounds.getY() + 2, 28, 26);

    // Layout for Plugin List Tab - list takes full space, button bar at bottom
    auto pluginListBounds = pluginListTab.getLocalBounds();

    // Create bottom button bar
    auto pluginButtonBar = pluginListBounds.removeFromBottom(50);
    refreshButton.setBounds(pluginButtonBar.removeFromLeft(140).reduced(8));

    // List takes remaining space with no padding
    pluginList.setBounds(pluginListBounds);

    // Layout for Search Paths Tab - list takes full space, button bar at bottom
    auto searchPathsBounds = searchPathsTab.getLocalBounds();

    // Create bottom button bar with all three buttons
    auto pathButtonBar = searchPathsBounds.removeFromBottom(50);
    addPathButton.setBounds(pathButtonBar.removeFromLeft(90).reduced(6));
    removePathButton.setBounds(pathButtonBar.removeFromLeft(120).reduced(6));
    resetToDefaultsButton.setBounds(pathButtonBar.removeFromRight(150).reduced(6));

    // List takes remaining space with no padding
    searchPathsList.setBounds(searchPathsBounds);
}

int PluginChainComponent::PluginBrowser::getNumRows() {
    // Show at least 1 row when loading to display the loading message
    if (isLoadingPlugins && pluginHost.getAvailablePlugins().size() == 0) {
        return 1;
    }
    return pluginHost.getAvailablePlugins().size();
}

void PluginChainComponent::PluginBrowser::paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                                                           bool rowIsSelected) {
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Modern list item styling with rounded corners and gradients
    if (rowIsSelected) {
        // Selected item with cyan gradient
        juce::ColourGradient selectedGradient(
            juce::Colour(0xff00d4ff).withAlpha(0.3f), bounds.getTopLeft(),
            juce::Colour(0xff0088cc).withAlpha(0.2f), bounds.getBottomLeft(),
            false
        );
        g.setGradientFill(selectedGradient);
        g.fillRect(bounds);

        // Bright border for selected item
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.8f));
        g.drawRect(bounds, 2.0f);
    } else {
        // Subtle dark background for non-selected items
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
        g.fillRect(bounds);

        // Subtle border
        g.setColour(juce::Colour(0xff333333).withAlpha(0.5f));
        g.drawRect(bounds, 1.0f);
    }

    if (isLoadingPlugins && rowNumber == 0) {
        // Show loading indicator with animated style
        g.setColour(juce::Colour(0xffffaa00)); // Orange for loading
        g.setFont(juce::Font("Arial", 14.0f, juce::Font::bold));
        juce::String loadingText = "SCANNING FOR PLUGINS...";
        g.drawText(loadingText, 24, 0, width - 48, height, juce::Justification::centredLeft);
    } else if (rowNumber < pluginHost.getAvailablePlugins().size()) {
        auto &plugin = pluginHost.getAvailablePlugins().getReference(rowNumber);

        auto textBounds = bounds.reduced(24, 8);
        
        // Create three-column layout: Plugin Name | Manufacturer | Format
        auto formatWidth = 80;  // Fixed width for format column
        auto manufacturerWidth = 150; // Fixed width for manufacturer column
        
        auto formatArea = textBounds.removeFromRight(formatWidth);
        auto manufacturerArea = textBounds.removeFromRight(manufacturerWidth);
        auto nameArea = textBounds; // Remaining space for plugin name
        
        // Plugin name in bold, larger font (left column)
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xfff0f0f0));
        g.setFont(juce::Font("Arial Black", 14.0f, juce::Font::bold));
        g.drawText(plugin.name.toUpperCase(), nameArea.toNearestInt(), juce::Justification::centredLeft);

        // Manufacturer in smaller font (middle column)
        g.setColour(rowIsSelected ? juce::Colour(0xffcccccc) : juce::Colour(0xffaaaaaa));
        g.setFont(juce::Font("Arial", 11.0f, juce::Font::plain));
        g.drawText(plugin.manufacturer, manufacturerArea.toNearestInt(), juce::Justification::centred);
        
        // Format in smaller font with color coding (right column)
        auto formatColor = getFormatColor(plugin.pluginFormatName, rowIsSelected);
        g.setColour(formatColor);
        g.setFont(juce::Font("Arial", 10.0f, juce::Font::bold));
        g.drawText(plugin.pluginFormatName.toUpperCase(), formatArea.toNearestInt(), juce::Justification::centred);
    }
}

void PluginChainComponent::PluginBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent &) {
    // Don't do anything if we're loading or if it's the loading indicator row
    if (isLoadingPlugins && row == 0 && pluginHost.getAvailablePlugins().size() == 0) {
        return;
    }

    if (row >= 0 && row < pluginHost.getAvailablePlugins().size()) {
        auto &plugin = pluginHost.getAvailablePlugins().getReference(row);
        pluginHost.loadPlugin(plugin);
        setVisible(false);
    }
}

void PluginChainComponent::PluginBrowser::buttonClicked(juce::Button *button) {
    if (button == &refreshButton) {
        refreshPluginList();
    } else if (button == &closeButton) {
        setVisible(false);
    } else if (button == &addPathButton) {
        showAddPathDialog();
    } else if (button == &removePathButton) {
        removeSelectedPath();
    } else if (button == &resetToDefaultsButton) {
        resetPathsToDefaults();
    }
}

void PluginChainComponent::PluginBrowser::refreshPluginList() {
    if (pluginHost.isPluginCacheValid()) {
        // Cache is valid, use it immediately
        DBG("Plugin cache is valid, using cached results");
        pluginList.updateContent();
        isLoadingPlugins = false;
    } else {
        // Cache is invalid, start async scan
        DBG("Plugin cache is invalid, starting async scan");
        isLoadingPlugins = true;
        pluginList.updateContent(); // Update to show loading state
        pluginHost.scanForPlugins(false);
    }
}

void PluginChainComponent::PluginBrowser::setVisible(bool shouldBeVisible) {
    Component::setVisible(shouldBeVisible);
    if (shouldBeVisible) {
        refreshPluginList();
        refreshSearchPathsList();
    }
}

void PluginChainComponent::PluginBrowser::onScanComplete() {
    DBG("PluginBrowser received scan complete notification");
    isLoadingPlugins = false;
    pluginList.updateContent();
    repaint(); // Refresh the UI
}

void PluginChainComponent::PluginBrowser::showAddPathDialog() {
    fileChooser = std::make_unique<juce::FileChooser>("Select Plugin Directory",
                                                      juce::File::getSpecialLocation(juce::File::userHomeDirectory));

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                             [this](const juce::FileChooser &fc) {
                                 auto result = fc.getResult();
                                 if (result.exists() && userConfig != nullptr) {
                                     userConfig->addVSTSearchPath(result.getFullPathName());
                                     refreshSearchPathsList();
                                     refreshPluginList();
                                 }
                                 // Clear the file chooser after use
                                 fileChooser.reset();
                             });
}

void PluginChainComponent::PluginBrowser::removeSelectedPath() {
    int selectedRow = searchPathsList.getSelectedRow();
    if (selectedRow >= 0 && userConfig != nullptr) {
        auto paths = userConfig->getVSTSearchPaths();
        if (selectedRow < paths.size()) {
            userConfig->removeVSTSearchPath(paths[selectedRow]);
            refreshSearchPathsList();
            refreshPluginList(); // Refresh plugins after removing path
        }
    }
}

void PluginChainComponent::PluginBrowser::resetPathsToDefaults() {
    if (userConfig != nullptr) {
        userConfig->clearVSTSearchPaths();
        auto defaultPaths = UserConfig::getDefaultVSTSearchPaths();
        userConfig->setVSTSearchPaths(defaultPaths);
        refreshSearchPathsList();
        refreshPluginList(); // Refresh plugins after resetting paths
    }
}

void PluginChainComponent::PluginBrowser::refreshSearchPathsList() {
    searchPathsList.updateContent();
    searchPathsList.repaint();
}

juce::Colour PluginChainComponent::PluginBrowser::getFormatColor(const juce::String& formatName, bool isSelected) {
    // Color coding for different plugin formats
    if (formatName.containsIgnoreCase("VST3")) {
        return isSelected ? juce::Colour(0xff00d4ff) : juce::Colour(0xff0088cc); // Cyan for VST3
    } else if (formatName.containsIgnoreCase("VST") && !formatName.containsIgnoreCase("VST3")) {
        return isSelected ? juce::Colour(0xffffaa00) : juce::Colour(0xffcc8800); // Orange for VST2
    } else if (formatName.containsIgnoreCase("AudioUnit") || formatName.containsIgnoreCase("AU")) {
        return isSelected ? juce::Colour(0xff88ff00) : juce::Colour(0xff66cc00); // Green for Audio Unit
    } else if (formatName.containsIgnoreCase("CLAP")) {
        return isSelected ? juce::Colour(0xffff6600) : juce::Colour(0xffcc4400); // Red-orange for CLAP
    } else {
        // Default color for unknown formats
        return isSelected ? juce::Colour(0xffcccccc) : juce::Colour(0xff888888); // Grey
    }
}

//==============================================================================
// SearchPathsListModel Implementation
//==============================================================================
int PluginChainComponent::PluginBrowser::SearchPathsListModel::getNumRows() {
    if (owner.userConfig != nullptr)
        return owner.userConfig->getVSTSearchPaths().size();
    return 0;
}

void PluginChainComponent::PluginBrowser::SearchPathsListModel::paintListBoxItem(int rowNumber, juce::Graphics &g,
                                                                                 int width, int height,
                                                                                 bool rowIsSelected) {
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Modern styling matching the plugin list
    if (rowIsSelected) {
        // Selected item with orange gradient (different from plugin list)
        juce::ColourGradient selectedGradient(
            juce::Colour(0xffffaa00).withAlpha(0.3f), bounds.getTopLeft(),
            juce::Colour(0xffcc8800).withAlpha(0.2f), bounds.getBottomLeft(),
            false
        );
        g.setGradientFill(selectedGradient);
        g.fillRect(bounds);

        // Bright border for selected item
        g.setColour(juce::Colour(0xffffaa00).withAlpha(0.8f));
        g.drawRect(bounds, 2.0f);
    } else {
        // Subtle dark background for non-selected items
        g.setColour(juce::Colour(0xff1a1a1a).withAlpha(0.8f));
        g.fillRect(bounds);

        // Subtle border
        g.setColour(juce::Colour(0xff333333).withAlpha(0.5f));
        g.drawRect(bounds, 1.0f);
    }

    // Modern text styling
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xfff0f0f0));
    g.setFont(juce::Font("Consolas", 12.0f, juce::Font::plain)); // Monospace for paths

    if (owner.userConfig != nullptr) {
        auto paths = owner.userConfig->getVSTSearchPaths();
        if (rowNumber < paths.size()) {
            // Add folder icon
            g.setColour(rowIsSelected ? juce::Colour(0xffffaa00) : juce::Colour(0xffaaaaaa));
            g.drawText(juce::String(rowNumber + 1) + ".", 16, 0, 20, height, juce::Justification::centred);

            // Draw path text
            g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xfff0f0f0));
            g.drawText(paths[rowNumber], 40, 0, width - 60, height, juce::Justification::centredLeft);
        }
    }
}

void PluginChainComponent::PluginBrowser::SearchPathsListModel::listBoxItemDoubleClicked(int row,
                                                                                         const juce::MouseEvent &) {
    // Could implement editing functionality here if needed
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================
PluginChainComponent::LevelMeter::LevelMeter() {
    smoothedLevel.reset(60.0, 0.1); // 60 fps, 100ms smoothing
}

PluginChainComponent::LevelMeter::~LevelMeter() = default;

void PluginChainComponent::LevelMeter::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);

    // Level bar
    float currentLevel = smoothedLevel.getNextValue();
    float levelHeight = bounds.getHeight() * currentLevel;

    juce::Rectangle<float> levelRect(bounds.getX(), bounds.getBottom() - levelHeight, bounds.getWidth(), levelHeight);

    // Color gradient based on level
    juce::Colour levelColour;
    if (currentLevel < 0.7f)
        levelColour = juce::Colours::green;
    else if (currentLevel < 0.9f)
        levelColour = juce::Colours::yellow;
    else
        levelColour = juce::Colours::red;

    g.setColour(levelColour);
    g.fillRect(levelRect);

    // Border
    g.setColour(juce::Colours::white);
    g.drawRect(bounds, 1.0f);
}

void PluginChainComponent::LevelMeter::setLevel(float newLevel) {
    level = juce::jlimit(0.0f, 1.0f, newLevel);
    smoothedLevel.setTargetValue(level.load());
    repaint();
}

//==============================================================================
// PluginChainContainer Implementation
//==============================================================================
void PluginChainComponent::PluginChainContainer::paint(juce::Graphics &g) {
    // Draw connecting lines between plugin slots
    g.setColour(juce::Colour(0xff666666).withAlpha(0.6f));

    for (int i = 0; i < parentComponent.pluginSlots.size() - 1; ++i) {
        auto* slot1 = parentComponent.pluginSlots[i];
        auto* slot2 = parentComponent.pluginSlots[i + 1];

        if (slot1->hasPlugin() && slot2 != nullptr) {
            auto slot1Bounds = slot1->getBounds();
            auto slot2Bounds = slot2->getBounds();

            juce::Line<float> line(slot1Bounds.getRight(), slot1Bounds.getCentreY(),
                                   slot2Bounds.getX(), slot2Bounds.getCentreY());
            g.drawLine(line, 2.0f);
        } else {
            break; // Stop drawing connections after first empty slot
        }
    }
}

void PluginChainComponent::PluginChainContainer::resized() {
    updateSlots();
}

void PluginChainComponent::PluginChainContainer::updateSlots() {
    int slotHeight = 60;
    int slotSpacing = 4;
    int totalHeight = 0;

    // Calculate total height needed
    if (!parentComponent.pluginSlots.isEmpty()) {
        totalHeight = parentComponent.pluginSlots.size() * (slotHeight + slotSpacing) - slotSpacing;
    }

    // Set container size
    auto viewportBounds = parentComponent.chainViewport.getBounds();
    int containerWidth = viewportBounds.getWidth() - parentComponent.chainViewport.getScrollBarThickness();
    setSize(containerWidth, juce::jmax(totalHeight, viewportBounds.getHeight()));

    // Layout slots vertically
    int yPos = 0;
    for (int i = 0; i < parentComponent.pluginSlots.size(); ++i) {
        auto* slot = parentComponent.pluginSlots[i];
        if (slot != nullptr) {
            slot->setBounds(0, yPos, containerWidth, slotHeight);
            yPos += slotHeight + slotSpacing;
        }
    }
}

//==============================================================================
// PluginChainComponent Implementation
//==============================================================================
PluginChainComponent::PluginBrowser *PluginChainComponent::getPluginBrowser() { return pluginBrowser.get(); }