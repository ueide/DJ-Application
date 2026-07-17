/*
    DeckGUI.h
    Deck UI component for transport controls, tempo/sync, looping, hot cues, and sampler pads.
*/


#pragma once
#include <optional> // Used for optional loop region callback
#include <utility>
#include <vector>
#include "FaderLookAndFeel.h"
#include <JuceHeader.h>


//--- A class for Customizing the Load Button LookAndFeel ---//
class LoadButtonLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        // Purpose: Provide a custom font size for the load button.
        // Inputs: button reference, buttonHeight in pixels.
        // Outputs: juce::Font for the button text.
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
        // Purpose: Draw the load button background based on interaction state.
        // Inputs: graphics context, button, background colour, mouse over flag, button down flag.
        // Outputs: none.
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour& backgroundColour,
                                bool isMouseOverButton, bool isButtonDown) override;
};


//--- Custom for arrow buttons (loop length adjustment) ---//
class ArrowButtonLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        // Purpose: Draw hover background for arrow buttons.
        // Inputs: graphics context, button, unused colour, mouse over flag, unused button down flag.
        // Outputs: none.
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool isMouseOverButton, bool) override
        {
            auto area = button.getLocalBounds().toFloat(); // button area
            if (isMouseOverButton) {
                g.setColour(juce::Colour(64, 64, 70));
                g.fillRoundedRectangle(area, 4.0f);
            }
        }

        // Purpose: Draw arrow button icon and hover state.
        // Inputs: graphics context, drawable button, mouse over flag, button down flag.
        // Outputs: none.
        void drawDrawableButton(juce::Graphics& g, juce::DrawableButton& button,
                                bool isMouseOver, bool isButtonDown) override
        {
            // Draw hover background first
            if (isMouseOver || isButtonDown) {
                auto area = button.getLocalBounds().toFloat(); // button area
                g.setColour(juce::Colour(64, 64, 70));
                g.fillRoundedRectangle(area, 4.0f);
            }
            // Draw the arrow icons
            juce::LookAndFeel_V4::drawDrawableButton(g, button, isMouseOver, isButtonDown);
        }
};


//--- Custom for standard toggle buttons (loop toggle, sync) ---//
class StandardToggleButtonLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        // Purpose: Create a toggle style with custom colors.
        // Inputs: standard, hover, active colours.
        // Outputs: initialized look-and-feel instance.
        StandardToggleButtonLookAndFeel(juce::Colour standard, juce::Colour hover, juce::Colour active)
            : standardColour(standard), hoverColour(hover), activeColour(active) {}

        // Purpose: Draw button background based on state.
        // Inputs: graphics context, button, unused colour, mouse over flag, button down flag.
        // Outputs: none.
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool isMouseOverButton, bool isButtonDown) override
        {
            auto area = button.getLocalBounds().toFloat();
            auto colour = button.getToggleState() || isButtonDown ? activeColour 
                        : isMouseOverButton ? hoverColour : standardColour;

            g.setColour(colour);
            g.fillRoundedRectangle(area, 4.0f);
            g.setColour(juce::Colour(78, 75, 75));
            g.drawRoundedRectangle(area, 4.0f, 1.0f);
        }

        // Purpose: Draw centered button text.
        // Inputs: graphics context, text button, unused flags.
        // Outputs: none.
        void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                            bool, bool) override
        {
            auto textColour = button.getToggleState() ? juce::Colour(1, 1, 1) : juce::Colour(215, 215, 215);
            g.setColour(textColour);
            g.setFont(juce::Font(10.0f ));
            g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
        }

    private:
        juce::Colour standardColour, hoverColour, activeColour;
};


//--- Custom for sync button with distinct active color ---//
class SyncButtonLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        // Purpose: Create a sync button style with custom colors.
        // Inputs: standard, hover, active colours.
        // Outputs: initialized look-and-feel instance.
        SyncButtonLookAndFeel(juce::Colour standard, juce::Colour hover, juce::Colour active)
            : standardColour(standard), hoverColour(hover), activeColour(active) {}

        // Purpose: Draw sync button background based on interaction.
        // Inputs: graphics context, button, unused colour, mouse over flag, button down flag.
        // Outputs: none.
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool isMouseOverButton, bool isButtonDown) override;
        
        // Purpose: Draw sync button text.
        // Inputs: graphics context, text button, mouse over flag, button down flag.
        // Outputs: none.
        void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                            bool isMouseOverButton, bool isButtonDown) override;
    
    private:
        juce::Colour standardColour, hoverColour, activeColour;
};

//--------------------- DeckGUI Class Definition ---------------------//
class DeckGUI  : public juce::Component,
                public juce::FileDragAndDropTarget,
                public juce::Slider::Listener,
                public juce::Timer,
                public juce::KeyListener {
    public:
        enum class DeckSide { Left, Right };
        
        // Purpose: Create a deck view.
        // Inputs: deckName for display, transportToUse for playback, side for layout mirroring.
        // Outputs: initialized DeckGUI instance.
        DeckGUI(juce::String deckName, juce::AudioTransportSource& transportToUse, DeckSide side = DeckSide::Left);

        // Purpose: Release deck resources.
        // Inputs: none.
        // Outputs: none.
        ~DeckGUI() override;

        // Purpose: Render the deck UI.
        // Inputs: graphics context.
        // Outputs: none.
        void paint (juce::Graphics&) override;
        // Purpose: Layout child components.
        // Inputs: none.
        // Outputs: none.
        void resized() override;
        
        // Purpose: Grab keyboard focus when added to hierarchy.
        // Inputs: none.
        // Outputs: none.
        void parentHierarchyChanged() override { grabKeyboardFocus(); }

        // Purpose: Accept file drags onto the deck.
        // Inputs: dragged file list.
        // Outputs: true if interested.
        bool isInterestedInFileDrag (const juce::StringArray& files) override {return true;}
        // Purpose: Handle dropped files and load to deck.
        // Inputs: dropped file list, drop position x/y.
        // Outputs: none.
        void filesDropped (const juce::StringArray& files, int x, int y) override;

        // Purpose: Update deck metadata labels for the current track.
        // Inputs: title, artist, analyzed BPM.
        // Outputs: none.
        void updateMetadata(juce::String title, juce::String artist, double bpm);

        // Notifies host when user loads a file from this deck UI.
        std::function<void(const juce::File&)> onFileLoaded;

        // Purpose: Periodically refresh UI state during playback.
        // Inputs: none.
        // Outputs: none.
        void timerCallback() override;
        // Purpose: Respond to slider value changes.
        // Inputs: slider pointer.
        // Outputs: none.
        void sliderValueChanged(juce::Slider* slider) override;
        // Purpose: Handle slider drag start.
        // Inputs: slider pointer.
        // Outputs: none.
        void sliderDragStarted(juce::Slider* slider) override;
        // Purpose: Handle slider drag end.
        // Inputs: slider pointer.
        // Outputs: none.
        void sliderDragEnded(juce::Slider* slider) override;
        
        // Purpose: Inject resampling source used for tempo and sync.
        // Inputs: resampling source pointer.
        // Outputs: none.
        void setResamplingSource(juce::ResamplingAudioSource* source) {
            resamplingSource = source;
            if (resamplingSource != nullptr) {
                resamplingSource->setResamplingRatio(baseSpeedRatio);
            }
        }

        std::function<void()> onSyncPressed;
        // Purpose: Access the current BPM value.
        // Inputs: none.
        // Outputs: current BPM.
        double getCurrentBPM() const {return currentBPM;};
        // Purpose: Adjust playback speed to match a target BPM.
        // Inputs: target BPM.
        // Outputs: none.
        void updateSpeedToMatchBPM(double targetBPM);

        // Notify host with active hot cue markers (timeSec, padIndex).
        std::function<void(const std::vector<std::pair<double, int>>&)> onHotCuesChanged;

        // Purpose: Store loaded file for persistence lookups.
        // Inputs: file reference.
        // Outputs: none.
        void setLoadedFile(const juce::File& file) { loadedFile = file; }
        // Purpose: Return the currently loaded file.
        // Inputs: none.
        // Outputs: file reference.
        juce::File getLoadedFile() const { return loadedFile; }

        // Purpose: Restore hot cues from time positions in seconds.
        // Inputs: cue times in seconds per pad.
        // Outputs: none.
        void setHotCuesFromSeconds(const std::vector<double>& cueSeconds);

        // Purpose: Set loop length in beats.
        // Inputs: beat count.
        // Outputs: none.
        void setLoopBeats(double beats);
        // Purpose: Activate loop using current loop points.
        // Inputs: none.
        // Outputs: none.
        void activateLoop();
        // Purpose: Disable the current loop.
        // Inputs: none.
        // Outputs: none.
        void disableLoop();
        // Purpose: Update loop labels and UI display.
        // Inputs: none.
        // Outputs: none.
        void updateLoopDisplay();
        // Purpose: Cycle loop length forward or backward.
        // Inputs: forward true/false.
        // Outputs: none.
        void cycleLoopBeats(bool forward);

        // Purpose: Notify host when loop region changes (seconds), or clear with nullopt.
        // Inputs: optional loop region (start, end) in seconds.
        // Outputs: none.
        std::function<void(std::optional<std::pair<double, double>>)> onLoopRegionChanged;

        // Purpose: Set sample rate for time/sample conversion.
        // Inputs: sample rate in Hz.
        // Outputs: none.
        void setSampleRate(double rate) { if (rate > 0.0) sampleRate = rate; }
        
        // Sampler integration callbacks.
        std::function<void(int padIndex, const juce::File&)> onSampleLoaded;
        std::function<void(int padIndex)> onSampleTrigger;
        std::function<void(int padIndex)> onSampleCleared;
        std::function<void()> onAllSamplesCleared;

        // Purpose: Restore sampler assignments from persisted files.
        // Inputs: sample files per pad (size up to 8).
        // Outputs: none.
        void setSamplerPadsFromFiles(const std::vector<juce::File>& files);

    private:
        juce::String name;
        DeckSide deckSide;
        juce::String currentTitle {"No Track Loaded"};
        juce::String currentArtist {"Unknown Artist"};
        double currentBPM {0.0};

        double playheadPosition {0.0};
        bool isPlaying {false};

        juce::AudioTransportSource& transport;
        juce::ResamplingAudioSource* resamplingSource {nullptr};
        double baseSpeedRatio {1.0};

        // Load button (UI)
        juce::TextButton loadButton {"Load"};
        LoadButtonLookAndFeel loadButtonLookAndFeel;

        // Sync button (UI)
        juce::TextButton syncButton {"Sync"};
        bool isSyncActive {false};
        SyncButtonLookAndFeel syncButtonLookAndFeel {juce::Colour(114, 212, 114),
                        juce::Colour(165, 207, 165),
                        juce::Colour(211, 177, 71)};


        //--- Loop Function ---//
        juce::Label loopLabel;
        juce::Label loopDisplay;
        juce::TextButton loopToggleButton {"In | Out"};
        std::unique_ptr<juce::DrawableButton> leftArrowButton;
        std::unique_ptr<juce::DrawableButton> rightArrowButton;
        std::unique_ptr<juce::Drawable> leftArrowIcon;
        std::unique_ptr<juce::Drawable> rightArrowIcon;
        ArrowButtonLookAndFeel arrowButtonLookAndFeel;
        StandardToggleButtonLookAndFeel standardToggleLookAndFeel {juce::Colour(26, 26, 28),
                                        juce::Colour(42, 42, 44),
                                        juce::Colour(211, 177, 71)};
        bool isLooping {false};
        juce::int64 loopStartSample {0};
        juce::int64 loopEndSample {0};
        double currentLoopBeats {4.0};
        double sampleRate {44100.0};

        void notifyLoopRegionChanged();
        void notifyHotCuesChanged();

        void clearAllHotCues();

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        
        bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

        //--- CUE button ---//
        juce::DrawableButton cueButton {"CUE", juce::DrawableButton::ImageFitted};
        bool isCueActive {false};
        bool cuePreviewing {false};
        bool hasCuePoint {false};
        juce::int64 cueSample {0};

        //--- Play/Pause button ---//
        juce::DrawableButton playPauseButton {"Play/Pause", juce::DrawableButton::ImageFitted};
        juce::Colour playPauseButtonColour {juce::Colour(183, 189, 184)};
        juce::Colour playPauseButtonHoverColour {juce::Colour(147, 148, 154)};
        juce::Colour playPauseButtonActiveColour {juce::Colour(211, 178, 71)};
        std::unique_ptr<juce::Drawable> playPauseIcon;

        //--- File choosers ---//
        std::unique_ptr<juce::FileChooser> fileChooser;
        std::unique_ptr<juce::FileChooser> sampleFileChooser;

        //--- Jog wheel slider ---//
        juce::Slider jogWheel;
        bool isDraggingJog {false};

        double lastJogValue = 0.0;

        //--- Performance Pad ---//
        static constexpr int numPads = 8;
        juce::TextButton padButtons[numPads];
        juce::TextButton hotCueButton {"Hot Cue"};
        juce::TextButton samplerButton {"Sampler"};
        juce::TextButton clearHotCuesButton {"Clear All"};
        
        enum class PadMode { None, HotCue, Sampler };
        PadMode currentPadMode {PadMode::None};
        
        // Pad state
        bool hasHotCue[numPads] {};
        juce::int64 hotCueSamples[numPads] {};
        int previewingHotCue {-1};

        juce::File loadedFile;
        
        // Hot cue functions
        void setHotCue(int padIndex);
        void jumpToHotCue(int padIndex);
        void clearHotCue(int padIndex);
        void triggerPad(int padIndex, bool isShiftDown);
        
        // Sampler pad state
        bool hasSample[numPads] {};
        juce::File sampleFiles[numPads];
        
        void loadSampleToPad(int padIndex);
        void playSample(int padIndex);
        void clearSample(int padIndex, bool notifyHost = true);
        void clearAllSamples(bool notifyHost = true);
        
        //--- Tempo Slider ---//
        juce::Slider tempoSlider;
        juce::Label tempoLabel;
        juce::Label tempoPerc;
        DJAppLookAndFeel faderLookAndFeel;

        // Layout constants
        static constexpr int CONTROLS_COLUMN_WIDTH = 56;
        static constexpr int CONTROLS_COLUMN_X = 456;
        static constexpr int BUTTON_SIZE_48 = 48;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeckGUI)
};