#include "PluginChainComponent.h"

//==============================================================================
PluginChainComponent::PluginChainComponent(VST3PluginHost& pluginHost_)
    : pluginHost(pluginHost_)
{
    // Initialize plugin slots
    for (int i = 0; i < maxPluginSlots; ++i)
    {
        pluginSlots[i] = std::make_unique<PluginSlot>(i, pluginHost, *this);
        addAndMakeVisible(*pluginSlots[i]);
    }
    
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
    pluginHost.onPluginError = [this](int index, const juce::String& error) { onPluginError(index, error); };
    
    // Start timer for updates
    startTimer(50); // Update every 50ms
    
    // Initial refresh
    refreshPluginChain();
}

PluginChainComponent::~PluginChainComponent()
{
    stopTimer();
    
    // Close all plugin editors
    for (int i = 0; i < maxPluginSlots; ++i)
    {
        closePluginEditor(i);
    }
}

//==============================================================================
void PluginChainComponent::paint(juce::Graphics& g)
{
    // Remove background fills to let the main app background show through
    // This makes the beautiful plugin slots stand out more
    
    // Draw connecting lines between plugin slots
    g.setColour(juce::Colour(0xff666666).withAlpha(0.6f));
    for (int i = 0; i < maxPluginSlots - 1; ++i)
    {
        if (pluginSlots[i]->hasPlugin())
        {
            auto slot1Bounds = pluginSlots[i]->getBounds();
            auto slot2Bounds = pluginSlots[i + 1]->getBounds();
            
            juce::Line<float> line(slot1Bounds.getRight(), slot1Bounds.getCentreY(),
                                  slot2Bounds.getX(), slot2Bounds.getCentreY());
            g.drawLine(line, 2.0f);
        }
        else
        {
            break; // Stop drawing connections after first empty slot
        }
    }
}

void PluginChainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Control area at top
    controlArea = area.removeFromTop(40);
    chainLabel.setBounds(controlArea.removeFromLeft(120));
    addPluginButton.setBounds(controlArea.removeFromLeft(100).reduced(2));
    clearAllButton.setBounds(controlArea.removeFromLeft(80).reduced(2));
    
    // Level meters are now handled by MainComponent
    // meterArea = area.removeFromRight(60);
    // for (int i = 0; i < 2; ++i)
    // {
    //     auto meterBounds = meterArea.removeFromTop(meterArea.getHeight() / 2);
    //     levelMeters[i]->setBounds(meterBounds.reduced(5));
    // }
    
    // Chain area (remaining space)
    chainArea = area.reduced(10);
    
    // Layout plugin slots
    int slotWidth = chainArea.getWidth() / maxPluginSlots;
    for (int i = 0; i < maxPluginSlots; ++i)
    {
        auto slotBounds = chainArea.removeFromLeft(slotWidth).reduced(2);
        pluginSlots[i]->setBounds(slotBounds);
    }
    
    // Plugin browser (full component)
    if (pluginBrowser)
    {
        pluginBrowser->setBounds(getLocalBounds());
    }
}

void PluginChainComponent::timerCallback()
{
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
void PluginChainComponent::refreshPluginChain()
{
    // Update plugin slots with current chain state
    for (int i = 0; i < maxPluginSlots; ++i)
    {
        if (i < pluginHost.getNumPlugins())
        {
            auto info = pluginHost.getPluginInfo(i);
            pluginSlots[i]->setPluginInfo(info);
        }
        else
        {
            pluginSlots[i]->clearPlugin();
        }
    }
    repaint();
}

void PluginChainComponent::showPluginBrowser()
{
    if (pluginBrowser)
    {
        pluginBrowser->refreshPluginList();
        pluginBrowser->setVisible(true);
        pluginBrowser->toFront(true);
    }
}

void PluginChainComponent::hidePluginBrowser()
{
    if (pluginBrowser)
    {
        pluginBrowser->setVisible(false);
    }
}

void PluginChainComponent::openPluginEditor(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= maxPluginSlots)
        return;
    
    // Close existing editor for this slot
    closePluginEditor(slotIndex);
    
    auto* editor = pluginHost.createEditorForPlugin(slotIndex);
    if (editor)
    {
        auto info = pluginHost.getPluginInfo(slotIndex);
        juce::String windowTitle = info.name + " - " + info.manufacturer;
        
        auto* window = new PluginEditorWindow(windowTitle, slotIndex, *this);
        window->setContentOwned(editor, true);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);
        
        editorWindows.add(window);
    }
}

void PluginChainComponent::closePluginEditor(int slotIndex)
{
    // Find and close the editor window for this slot
    for (int i = editorWindows.size() - 1; i >= 0; --i)
    {
        auto* window = editorWindows[i];
        if (window->getSlotIndex() == slotIndex)
        {
            // Close the VST3PluginHost editor first
            pluginHost.closeEditorForPlugin(slotIndex);
            
            // Remove the window (this will delete it safely)
            editorWindows.remove(i);
            break;
        }
    }
}

void PluginChainComponent::onEditorWindowClosed(int slotIndex)
{
    // Called when user clicks the close button on a plugin editor window
    // This ensures proper cleanup when the window is closed by the user
    juce::MessageManager::callAsync([this, slotIndex]()
    {
        closePluginEditor(slotIndex);
    });
}

void PluginChainComponent::onPluginChainChanged()
{
    refreshPluginChain();
}

void PluginChainComponent::onPluginError(int pluginIndex, const juce::String& error)
{
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Plugin Error",
        "Plugin " + juce::String(pluginIndex) + ": " + error,
        "OK"
    );
}

void PluginChainComponent::handleDraggedPlugin(int fromSlot, int toSlot)
{
    // Convert UI slot indices to actual plugin chain indices
    // Only move if both slots have plugins or we're moving to an empty slot
    if (fromSlot >= 0 && fromSlot < maxPluginSlots && 
        toSlot >= 0 && toSlot < maxPluginSlots &&
        fromSlot != toSlot)
    {
        // Check if source slot has a plugin
        if (fromSlot < pluginHost.getNumPlugins())
        {
            // If target slot is beyond current plugins, we're appending
            if (toSlot >= pluginHost.getNumPlugins())
            {
                // Move to end of chain
                pluginHost.movePlugin(fromSlot, pluginHost.getNumPlugins() - 1);
            }
            else
            {
                // Move within existing chain
                pluginHost.movePlugin(fromSlot, toSlot);
            }
        }
    }
}

//==============================================================================
// PluginSlot Implementation
//==============================================================================
PluginChainComponent::PluginSlot::PluginSlot(int index, VST3PluginHost& host, PluginChainComponent& parent)
    : slotIndex(index), pluginHost(host), parentComponent(parent)
{
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(manufacturerLabel);
    addAndMakeVisible(editButton);
    addAndMakeVisible(removeButton);
    
    // Setup labels with dark theme
    nameLabel.setJustificationType(juce::Justification::centredTop);
    nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    nameLabel.setInterceptsMouseClicks(false, false); // Allow mouse events to pass through
    
    manufacturerLabel.setJustificationType(juce::Justification::centredTop);
    manufacturerLabel.setFont(juce::Font(10.0f));
    manufacturerLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
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

PluginChainComponent::PluginSlot::~PluginSlot()
{
    editButton.removeListener(this);
    removeButton.removeListener(this);
}

void PluginChainComponent::PluginSlot::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Draw drag over highlight first (behind everything else)
    if (isDragOver)
    {
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.3f));
        g.fillRect(bounds);
        
        g.setColour(juce::Colour(0xff00d4ff));
        g.drawRect(bounds, 3);
    }
    
    if (hasPlugin())
    {
        // Store original bounds for shadow
        auto originalBounds = bounds;
        
        // Draw enhanced plugin background
        drawPluginBackground(g, bounds);
        
        // Draw status indicator at the top center
        drawStatusIndicator(g, bounds);
        
        // Draw plugin icon area in top portion (below status indicator)
        auto iconArea = bounds.removeFromTop(bounds.getHeight() / 3).reduced(8);
        iconArea.removeFromTop(12); // Make room for status indicator
        drawPluginIcon(g, iconArea);
        
        // Add subtle shadow effect when not bypassed - use original bounds
        if (!isBypassed)
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.drawRect(originalBounds.expanded(1), 1);
        }
    }
    else
    {
        // Draw empty slot with subtle pattern
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(bounds);
        
        // Draw dashed border for empty slots
        g.setColour(juce::Colour(0xff404040));
        float dashLengths[] = {4.0f, 4.0f};
        auto borderBounds = bounds.toFloat();
        g.drawDashedLine(juce::Line<float>(borderBounds.getX(), borderBounds.getY(), 
                                          borderBounds.getRight(), borderBounds.getY()), 
                        dashLengths, 2);
        g.drawDashedLine(juce::Line<float>(borderBounds.getRight(), borderBounds.getY(), 
                                          borderBounds.getRight(), borderBounds.getBottom()), 
                        dashLengths, 2);
        g.drawDashedLine(juce::Line<float>(borderBounds.getRight(), borderBounds.getBottom(), 
                                          borderBounds.getX(), borderBounds.getBottom()), 
                        dashLengths, 2);
        g.drawDashedLine(juce::Line<float>(borderBounds.getX(), borderBounds.getBottom(), 
                                          borderBounds.getX(), borderBounds.getY()), 
                        dashLengths, 2);
        
        // Draw "+" icon for empty slots
        g.setColour(juce::Colour(0xff666666));
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        auto iconSize = 20;
        g.drawLine(centerX - iconSize/2, centerY, centerX + iconSize/2, centerY, 2.0f);
        g.drawLine(centerX, centerY - iconSize/2, centerX, centerY + iconSize/2, 2.0f);
        
        g.setColour(juce::Colour(0xff666666));
        g.drawText("Empty", bounds.reduced(4), juce::Justification::centredBottom);
    }
}

void PluginChainComponent::PluginSlot::resized()
{
    if (!hasPlugin())
        return;
    
    auto bounds = getLocalBounds().reduced(4);
    
    // Status indicator at top center
    statusIndicatorBounds = juce::Rectangle<int>(bounds.getCentreX() - 6, bounds.getY() + 6, 12, 12);
    
    // Reserve top area for icon and status indicator
    bounds.removeFromTop(bounds.getHeight() / 3);
    
    // Labels in middle area (more space now)
    auto labelArea = bounds.removeFromTop(bounds.getHeight() * 0.5f);
    nameLabel.setBounds(labelArea.removeFromTop(labelArea.getHeight() / 2));
    manufacturerLabel.setBounds(labelArea);
    
    // Buttons at bottom with fixed height, stacked from bottom
    auto buttonArea = bounds.reduced(2);
    auto buttonHeight = 50;  // Taller buttons
    auto buttonSpacing = 4;
    
    // Stack buttons from the bottom
    removeButton.setBounds(buttonArea.removeFromBottom(buttonHeight).reduced(1));
    buttonArea.removeFromBottom(buttonSpacing);
    editButton.setBounds(buttonArea.removeFromBottom(buttonHeight).reduced(1));
}

void PluginChainComponent::PluginSlot::buttonClicked(juce::Button* button)
{
    if (!hasPlugin())
        return;
    
    if (button == &editButton)
    {
        parentComponent.openPluginEditor(slotIndex);
    }
    else if (button == &removeButton)
    {
        pluginHost.unloadPlugin(slotIndex);
    }
}

void PluginChainComponent::PluginSlot::mouseDown(const juce::MouseEvent& event)
{
    if (!hasPlugin())
    {
        // Clicking on empty slot opens the plugin browser
        parentComponent.showPluginBrowser();
        return;
    }
    
    // Store the mouse down position for later use in mouseDrag
    mouseDownPosition = event.getPosition();
}

void PluginChainComponent::PluginSlot::mouseUp(const juce::MouseEvent& event)
{
    if (!hasPlugin())
        return;
    
    // Only process click if we haven't moved much (not a drag)
    if (event.getDistanceFromDragStart() < 5)
    {
        // Check if click is within the status indicator bounds
        if (statusIndicatorBounds.contains(event.getPosition()))
        {
            bool currentlyBypassed = pluginHost.isPluginBypassed(slotIndex);
            pluginHost.bypassPlugin(slotIndex, !currentlyBypassed);
            updateBypassState();
        }
    }
}

void PluginChainComponent::PluginSlot::mouseDrag(const juce::MouseEvent& event)
{
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

void PluginChainComponent::PluginSlot::mouseMove(const juce::MouseEvent& event)
{
    if (!hasPlugin())
    {
        // Show pointer cursor for empty slots to indicate they're clickable
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        return;
    }
    
    bool wasHovered = isStatusIndicatorHovered;
    isStatusIndicatorHovered = statusIndicatorBounds.contains(event.getPosition());
    
    // Determine cursor based on what we're hovering over
    if (isStatusIndicatorHovered)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    else if ((editButton.isVisible() && editButton.getBounds().contains(event.getPosition())) || 
             (removeButton.isVisible() && removeButton.getBounds().contains(event.getPosition())))
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor); // Buttons are clickable
    }
    else
    {
        // Over the main plugin area - show drag cursor to indicate it's draggable
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    
    // Only repaint if hover state changed
    if (wasHovered != isStatusIndicatorHovered)
    {
        repaint();
    }
}

void PluginChainComponent::PluginSlot::mouseExit(const juce::MouseEvent& event)
{
    // Always reset cursor when exiting
    setMouseCursor(juce::MouseCursor::NormalCursor);
    
    if (isStatusIndicatorHovered)
    {
        isStatusIndicatorHovered = false;
        repaint();
    }
}

//==============================================================================
// Drag and Drop Target Implementation
//==============================================================================
bool PluginChainComponent::PluginSlot::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // We're interested in plugin slot drags (identified by integer slot index)
    return dragSourceDetails.description.isInt();
}

void PluginChainComponent::PluginSlot::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    isDragOver = true;
    repaint();
}

void PluginChainComponent::PluginSlot::itemDragMove(const SourceDetails& dragSourceDetails)
{
    // Visual feedback is handled by isDragOver flag
}

void PluginChainComponent::PluginSlot::itemDragExit(const SourceDetails& dragSourceDetails)
{
    isDragOver = false;
    repaint();
}

void PluginChainComponent::PluginSlot::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragOver = false;
    
    if (dragSourceDetails.description.isInt())
    {
        int fromSlotIndex = dragSourceDetails.description;
        int toSlotIndex = slotIndex;
        
        if (fromSlotIndex != toSlotIndex)
        {
            parentComponent.handleDraggedPlugin(fromSlotIndex, toSlotIndex);
        }
    }
    
    repaint();
}

void PluginChainComponent::PluginSlot::setPluginInfo(const VST3PluginHost::PluginInfo& info)
{
    pluginInfo = info;
    
    nameLabel.setText(info.name, juce::dontSendNotification);
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

void PluginChainComponent::PluginSlot::clearPlugin()
{
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

void PluginChainComponent::PluginSlot::updateBypassState()
{
    if (hasPlugin())
    {
        isBypassed = pluginHost.isPluginBypassed(slotIndex);
        repaint();  // Repaint to update the status indicator appearance
    }
}

void PluginChainComponent::PluginSlot::generatePluginTheme()
{
    if (!hasPlugin()) return;
    
    // Generate colors based on plugin name and manufacturer using hash
    primaryColour = getHashBasedColour(pluginInfo.manufacturer, 0.7f, 0.6f);
    secondaryColour = getHashBasedColour(pluginInfo.name, 0.6f, 0.4f);
    
    // Create accent color by blending primary and secondary
    accentColour = primaryColour.interpolatedWith(secondaryColour, 0.5f).withBrightness(0.8f);
    
    // Ensure colors work well with dark theme
    primaryColour = primaryColour.withSaturation(0.7f).withBrightness(0.6f);
    secondaryColour = secondaryColour.withSaturation(0.6f).withBrightness(0.4f);
}

void PluginChainComponent::PluginSlot::drawPluginBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Always draw procedural pattern
    drawProceduralPattern(g, bounds);
    
    // Draw border
    g.setColour(accentColour.withAlpha(isBypassed ? 0.3f : 0.8f));
    g.drawRect(bounds, 2);
}

void PluginChainComponent::PluginSlot::drawProceduralPattern(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Create gradient background
    juce::ColourGradient gradient(primaryColour.withAlpha(isBypassed ? 0.3f : 0.7f), 
                                 bounds.getX(), bounds.getY(),
                                 secondaryColour.withAlpha(isBypassed ? 0.2f : 0.5f), 
                                 bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);
    
    // Add pattern based on plugin type and name
    g.setColour(accentColour.withAlpha(isBypassed ? 0.1f : 0.2f));
    
    // Generate pattern based on plugin name hash
    auto nameHash = pluginInfo.name.hashCode();
    auto patternType = nameHash % 4;
    
    switch (patternType)
    {
        case 0: // Diagonal lines
        {
            for (int i = -bounds.getHeight(); i < bounds.getWidth(); i += 20)
            {
                g.drawLine(bounds.getX() + i, bounds.getY(), 
                          bounds.getX() + i + bounds.getHeight(), bounds.getBottom(), 1.0f);
            }
            break;
        }
        case 1: // Dots pattern
        {
            for (int x = bounds.getX() + 10; x < bounds.getRight(); x += 15)
            {
                for (int y = bounds.getY() + 10; y < bounds.getBottom(); y += 15)
                {
                    g.fillEllipse(x - 1, y - 1, 2, 2);
                }
            }
            break;
        }
        case 2: // Wave pattern
        {
            juce::Path wavePath;
            wavePath.startNewSubPath(bounds.getX(), bounds.getCentreY());
            for (int x = bounds.getX(); x < bounds.getRight(); x += 5)
            {
                float y = bounds.getCentreY() + 10 * std::sin((x - bounds.getX()) * 0.1f);
                wavePath.lineTo(x, y);
            }
            g.strokePath(wavePath, juce::PathStrokeType(1.5f));
            break;
        }
        case 3: // Grid pattern
        {
            for (int x = bounds.getX(); x < bounds.getRight(); x += 25)
            {
                g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 0.5f);
            }
            for (int y = bounds.getY(); y < bounds.getBottom(); y += 25)
            {
                g.drawLine(bounds.getX(), y, bounds.getRight(), y, 0.5f);
            }
            break;
        }
    }
}

void PluginChainComponent::PluginSlot::drawPluginIcon(juce::Graphics& g, const juce::Rectangle<int>& iconArea)
{
    if (iconArea.isEmpty()) return;
    
    // Draw icon based on plugin type
    g.setColour(accentColour.withAlpha(isBypassed ? 0.5f : 1.0f));
    
    if (pluginInfo.isInstrument)
    {
        // Draw musical note icon for instruments
        juce::Path notePath;
        auto center = iconArea.getCentre();
        auto size = juce::jmin(iconArea.getWidth(), iconArea.getHeight()) * 0.6f;
        
        // Musical note shape
        notePath.addEllipse(center.x - size * 0.2f, center.y, size * 0.4f, size * 0.3f);
        notePath.addRectangle(center.x + size * 0.15f, center.y - size * 0.4f, size * 0.1f, size * 0.7f);
        
        g.fillPath(notePath);
    }
    else
    {
        // Draw waveform icon for effects
        juce::Path wavePath;
        auto startX = iconArea.getX() + iconArea.getWidth() * 0.1f;
        auto endX = iconArea.getRight() - iconArea.getWidth() * 0.1f;
        auto centerY = iconArea.getCentreY();
        auto amplitude = iconArea.getHeight() * 0.3f;
        
        wavePath.startNewSubPath(startX, centerY);
        for (float x = startX; x <= endX; x += 2.0f)
        {
            float progress = (x - startX) / (endX - startX);
            float y = centerY + amplitude * std::sin(progress * 6.28f * 2); // 2 cycles
            wavePath.lineTo(x, y);
        }
        
        g.strokePath(wavePath, juce::PathStrokeType(2.0f));
    }
}

juce::Colour PluginChainComponent::PluginSlot::getHashBasedColour(const juce::String& text, float saturation, float brightness)
{
    // Create a hash from the text
    auto hash = text.hashCode();
    
    // Use hash to generate hue (0-360 degrees)
    float hue = (hash % 360) / 360.0f;
    
    return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

void PluginChainComponent::PluginSlot::drawStatusIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (statusIndicatorBounds.isEmpty()) return;
    
    auto indicatorBounds = statusIndicatorBounds.toFloat();
    
    // Enhanced glow effect when hovered
    if (isStatusIndicatorHovered)
    {
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.3f));
        g.fillEllipse(indicatorBounds.expanded(6));
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.5f));
        g.fillEllipse(indicatorBounds.expanded(3));
    }
    
    // Draw glow effect when active
    if (!isBypassed)
    {
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
    if (!isBypassed)
    {
        // Draw a bright center dot
        auto centerBounds = indicatorBounds.reduced(3);
        g.setColour(juce::Colour(0xffffffff).withAlpha(isStatusIndicatorHovered ? 1.0f : 0.9f));
        g.fillEllipse(centerBounds);
        
        // Add pulsing ring effect
        g.setColour(juce::Colour(0xff00ff88).withAlpha(0.8f));
        g.drawEllipse(indicatorBounds.reduced(1), isStatusIndicatorHovered ? 2.0f : 1.5f);
    }
    else
    {
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

//==============================================================================
// PluginBrowser Implementation
//==============================================================================
PluginChainComponent::PluginBrowser::PluginBrowser(VST3PluginHost& host)
    : pluginHost(host)
{
    addAndMakeVisible(headerLabel);
    addAndMakeVisible(pluginList);
    addAndMakeVisible(refreshButton);
    addAndMakeVisible(closeButton);
    
    headerLabel.setText("Available VST3 Plugins", juce::dontSendNotification);
    headerLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    headerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    refreshButton.setButtonText("Refresh");
    closeButton.setButtonText("Close");
    
    // Apply modern theme to buttons - they'll use the LookAndFeel gradients
    refreshButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    refreshButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    
    // Apply transparent theme to list box for clean integration
    pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    pluginList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    
    refreshButton.addListener(this);
    closeButton.addListener(this);
    
    pluginList.setModel(this);
    pluginList.setMultipleSelectionEnabled(false);
    pluginList.setRowHeight(40); // Taller rows for better padding
}

PluginChainComponent::PluginBrowser::~PluginBrowser()
{
    refreshButton.removeListener(this);
    closeButton.removeListener(this);
    pluginList.setModel(nullptr);
}

void PluginChainComponent::PluginBrowser::paint(juce::Graphics& g)
{
    // Black background matching the main app
    g.fillAll(juce::Colour(0xff0a0a0a));
}

void PluginChainComponent::PluginBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(30);
    
    headerLabel.setBounds(bounds.removeFromTop(30));
    
    auto buttonArea = bounds.removeFromBottom(40);
    refreshButton.setBounds(buttonArea.removeFromLeft(100).reduced(5));
    closeButton.setBounds(buttonArea.removeFromRight(100).reduced(5));
    
    pluginList.setBounds(bounds.reduced(10));
}

int PluginChainComponent::PluginBrowser::getNumRows()
{
    return pluginHost.getAvailablePlugins().size();
}

void PluginChainComponent::PluginBrowser::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Modern list item styling with rounded corners and more padding
    if (rowIsSelected)
    {
        // Selected item with white highlight
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.reduced(4), 4.0f);
        
        // Subtle border for selected item
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(4), 4.0f, 1.0f);
    }
    else
    {
        // Subtle hover effect for non-selected items
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRoundedRectangle(bounds.reduced(4), 4.0f);
    }
    
    // Modern text styling with more padding
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffe0e0e0));
    
    if (rowNumber < pluginHost.getAvailablePlugins().size())
    {
        auto& plugin = pluginHost.getAvailablePlugins().getReference(rowNumber);
        
        juce::String text = plugin.name + " - " + plugin.manufacturer;
        g.drawText(text, 20, 0, width - 40, height, juce::Justification::centredLeft);
    }
}

void PluginChainComponent::PluginBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < pluginHost.getAvailablePlugins().size())
    {
        auto& plugin = pluginHost.getAvailablePlugins().getReference(row);
        pluginHost.loadPlugin(plugin);
        setVisible(false);
    }
}

void PluginChainComponent::PluginBrowser::buttonClicked(juce::Button* button)
{
    if (button == &refreshButton)
    {
        refreshPluginList();
    }
    else if (button == &closeButton)
    {
        setVisible(false);
    }
}

void PluginChainComponent::PluginBrowser::refreshPluginList()
{
    pluginHost.scanForPlugins();
    pluginList.updateContent();
}

void PluginChainComponent::PluginBrowser::setVisible(bool shouldBeVisible)
{
    Component::setVisible(shouldBeVisible);
    if (shouldBeVisible)
    {
        refreshPluginList();
    }
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================
PluginChainComponent::LevelMeter::LevelMeter()
{
    smoothedLevel.reset(60.0, 0.1); // 60 fps, 100ms smoothing
}

PluginChainComponent::LevelMeter::~LevelMeter() = default;

void PluginChainComponent::LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);
    
    // Level bar
    float currentLevel = smoothedLevel.getNextValue();
    float levelHeight = bounds.getHeight() * currentLevel;
    
    juce::Rectangle<float> levelRect(bounds.getX(), 
                                    bounds.getBottom() - levelHeight,
                                    bounds.getWidth(), 
                                    levelHeight);
    
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

void PluginChainComponent::LevelMeter::setLevel(float newLevel)
{
    level = juce::jlimit(0.0f, 1.0f, newLevel);
    smoothedLevel.setTargetValue(level.load());
    repaint();
} 