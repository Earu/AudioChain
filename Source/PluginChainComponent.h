#pragma once

#include <JuceHeader.h>
#include "VST3PluginHost.h"
#include "UserConfig.h"

//==============================================================================
/**
    Plugin Chain Component - UI for managing the VST3 plugin chain
    
    Features:
    - Visual representation of plugin chain
    - Drag and drop plugin ordering
    - Plugin controls (bypass, remove, edit)
    - Plugin browser and loading
    - Real-time level meters
    - Spectrum analyzer
*/
class PluginChainComponent : public juce::Component,
                            public juce::DragAndDropContainer,
                            public juce::Timer
{
public:
    explicit PluginChainComponent(VST3PluginHost& pluginHost);
    ~PluginChainComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Timer callback for updates
    void timerCallback() override;
    
private:
    //==============================================================================
    class PluginSlot : public juce::Component,
                       public juce::Button::Listener,
                       public juce::DragAndDropTarget
    {
    public:
        PluginSlot(int index, VST3PluginHost& host, PluginChainComponent& parent);
        ~PluginSlot() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void buttonClicked(juce::Button* button) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        
        // DragAndDropTarget overrides
        bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
        void itemDragEnter(const SourceDetails& dragSourceDetails) override;
        void itemDragMove(const SourceDetails& dragSourceDetails) override;
        void itemDragExit(const SourceDetails& dragSourceDetails) override;
        void itemDropped(const SourceDetails& dragSourceDetails) override;
        
        void setPluginInfo(const VST3PluginHost::PluginInfo& info);
        void clearPlugin();
        void updateBypassState();
        
        int getIndex() const { return slotIndex; }
        bool hasPlugin() const { return !pluginInfo.name.isEmpty(); }
        
    private:
        int slotIndex;
        VST3PluginHost& pluginHost;
        PluginChainComponent& parentComponent;
        
        VST3PluginHost::PluginInfo pluginInfo;
        
        // UI Elements
        juce::Label nameLabel;
        juce::Label manufacturerLabel;
        juce::TextButton editButton;
        juce::TextButton removeButton;
        
        // Visual elements
        bool isBypassed = false;
        bool isStatusIndicatorHovered = false;
        bool isDragOver = false;
        juce::Point<int> mouseDownPosition;
        juce::Colour slotColour;
        juce::Colour primaryColour;
        juce::Colour secondaryColour;
        juce::Colour accentColour;
        juce::Rectangle<int> statusIndicatorBounds;  // For the status circle
        
        // Visual enhancement methods
        void generatePluginTheme();
        void drawPluginBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds);
        void drawProceduralPattern(juce::Graphics& g, const juce::Rectangle<int>& bounds);
        void drawPluginIcon(juce::Graphics& g, const juce::Rectangle<int>& iconArea);
        void drawStatusIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds);
        juce::Colour getHashBasedColour(const juce::String& text, float saturation = 0.7f, float brightness = 0.8f);
        juce::Colour getPluginTypeColour(bool isInstrument);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSlot)
    };
    
    //==============================================================================
    class PluginBrowser : public juce::Component,
                         public juce::ListBoxModel,
                         public juce::Button::Listener
    {
    public:
        explicit PluginBrowser(VST3PluginHost& host);
        ~PluginBrowser() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // ListBoxModel overrides
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
        
        // Button::Listener override
        void buttonClicked(juce::Button* button) override;
        
        void refreshPluginList();
        void setVisible(bool shouldBeVisible) override;
        void setUserConfig(UserConfig* config) { userConfig = config; }
        
    private:
        VST3PluginHost& pluginHost;
        UserConfig* userConfig = nullptr;
        
        // Main UI components  
        juce::ListBox pluginList;
        juce::TextButton refreshButton;
        juce::TextButton closeButton;
        juce::Label headerLabel;
        
        // Search path management UI
        juce::TabbedComponent tabs;
        juce::Component pluginListTab;
        juce::Component searchPathsTab;
        
        // Search paths tab components
        juce::ListBox searchPathsList;
        juce::TextButton addPathButton;
        juce::TextButton removePathButton;
        juce::TextButton resetToDefaultsButton;
        juce::Label searchPathsLabel;
        
        // Search paths ListBoxModel
        class SearchPathsListModel : public juce::ListBoxModel
        {
        public:
            SearchPathsListModel(PluginBrowser& owner) : owner(owner) {}
            
            int getNumRows() override;
            void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
            void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
            
        private:
            PluginBrowser& owner;
        };
        
        SearchPathsListModel searchPathsModel;
        
        // File chooser (needs to stay alive during async operation)
        std::unique_ptr<juce::FileChooser> fileChooser;
        
        // Helper methods
        void showAddPathDialog();
        void removeSelectedPath();
        void resetPathsToDefaults();
        void refreshSearchPathsList();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
    };
    
    //==============================================================================
public:
    // Access to plugin browser for configuration  
    PluginBrowser* getPluginBrowser();
    
private:
    class LevelMeter : public juce::Component
    {
    public:
        LevelMeter();
        ~LevelMeter() override;
        
        void paint(juce::Graphics& g) override;
        void setLevel(float newLevel);
        
    private:
        std::atomic<float> level{0.0f};
        juce::LinearSmoothedValue<float> smoothedLevel;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
    };
    
    //==============================================================================
    VST3PluginHost& pluginHost;
    
    // Plugin slots
    static constexpr int maxPluginSlots = 8;
    std::array<std::unique_ptr<PluginSlot>, maxPluginSlots> pluginSlots;
    
    // UI Components
    std::unique_ptr<PluginBrowser> pluginBrowser;
    juce::TextButton addPluginButton;
    juce::TextButton clearAllButton;
    juce::Label chainLabel;
    
    // Metering
    std::array<std::unique_ptr<LevelMeter>, 2> levelMeters; // L/R channels
    
    // Layout
    juce::Rectangle<int> chainArea;
    juce::Rectangle<int> controlArea;
    juce::Rectangle<int> meterArea;
    
    // Plugin editor windows
    class PluginEditorWindow : public juce::DocumentWindow
    {
    public:
        PluginEditorWindow(const juce::String& title, int pluginIndex, PluginChainComponent& parent)
            : DocumentWindow(title, juce::Colours::darkgrey, DocumentWindow::allButtons),
              slotIndex(pluginIndex), parentComponent(parent)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
        }
        
        void closeButtonPressed() override
        {
            parentComponent.onEditorWindowClosed(slotIndex);
        }
        
        int getSlotIndex() const { return slotIndex; }
        
    private:
        int slotIndex;
        PluginChainComponent& parentComponent;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
    };
    
    juce::OwnedArray<PluginEditorWindow> editorWindows;
    
    // Drag and drop
    void handleDraggedPlugin(int fromSlot, int toSlot);
    
    // Plugin management
    void refreshPluginChain();
    void showPluginBrowser();
    void hidePluginBrowser();
    void openPluginEditor(int slotIndex);
    void closePluginEditor(int slotIndex);
    void onEditorWindowClosed(int slotIndex);
    
    // Callbacks
    void onPluginChainChanged();
    void onPluginError(int pluginIndex, const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainComponent)
}; 