/*
    MainComponent.h
    Central coordinator for the DJ App application.
    It is responsible for managing audio playback, UI layout, and state persistence.
*/


#pragma once
#include <JuceHeader.h>
#include <array>
#include <map>
#include <vector>
#include "WaveformDisplay.h"
#include "MixerControls.h"
#include "DeckGUI.h"
#include "AudioAnalyzer.h"
#include "Playlist.h"


//--- MainComponent class definition ---//
class MainComponent  : public juce::AudioAppComponent,
                    private juce::Timer {
    public:
        // Purpose: Construct the main application component.
        // Inputs: none.
        // Outputs: initialized MainComponent instance.
        MainComponent();
        // Purpose: Release main component resources.
        // Inputs: none.
        // Outputs: none.
        ~MainComponent() override;

        // Purpose: Prepare audio sources before playback.
        // Inputs: samplesPerBlockExpected, sampleRate in Hz.
        // Outputs: none.
        void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
        // Purpose: Fill the next audio buffer for playback.
        // Inputs: bufferToFill with output buffer information.
        // Outputs: none.
        void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
        // Purpose: Release audio resources after playback.
        // Inputs: none.
        // Outputs: none.
        void releaseResources() override;

        // Purpose: Render the main UI.
        // Inputs: graphics context.
        // Outputs: none.
        void paint (juce::Graphics& g) override;
        // Purpose: Layout main UI components.
        // Inputs: none.
        // Outputs: none.
        void resized() override;

        // Purpose: Refresh UI playback state on a timer.
        // Inputs: none.
        // Outputs: none.
        void timerCallback() override;

    private:
        const int baseWidth  = 1280;
        const int baseHeight = 780;

        // Layout areas
        juce::Rectangle<int> waveformArea;
        juce::Rectangle<int> libraryArea;
        juce::Rectangle<int> decksArea;

        juce::AudioFormatManager formatManager;
        juce::AudioThumbnailCache thumbnailCache {50};

        juce::AudioFormatReaderSource* readerSourceLeft {nullptr};
        juce::ResamplingAudioSource* resamplingSourceLeft {nullptr}; 
        juce::AudioTransportSource transportSourceLeft;

        juce::AudioFormatReaderSource* readerSourceRight {nullptr};
        juce::ResamplingAudioSource* resamplingSourceRight {nullptr}; 
        juce::AudioTransportSource transportSourceRight;
        
        // Sampler pads for each deck
        static constexpr int numSamplerPads = 8;
        juce::AudioFormatReaderSource* sampleReaderLeft[numSamplerPads] {};
        juce::AudioTransportSource sampleTransportLeft[numSamplerPads];
        juce::AudioFormatReaderSource* sampleReaderRight[numSamplerPads] {};
        juce::AudioTransportSource sampleTransportRight[numSamplerPads];

        // Load a track into deck index (0 = left, 1 = right).
        void loadFileIntoDeck(const juce::File& file, int deckIndex);
        // Load a sample to a sampler pad for a given deck.
        void loadSampleToPad(int deckIndex, int padIndex, const juce::File& file);
        // Clear a sample from a sampler pad for a given deck.
        void clearSamplePad(int deckIndex, int padIndex);
        // Trigger playback of a loaded sampler pad.
        void triggerSample(int deckIndex, int padIndex);

        AudioAnalyzer::AnalysisResult lastAnalysisResultLeft;
        AudioAnalyzer::AnalysisResult lastAnalysisResultRight;

        WaveformDisplay waveformLeft {formatManager, thumbnailCache, "Left"};
        WaveformDisplay waveformRight {formatManager, thumbnailCache, "Right"};

        MixerControls mixerControls;
        Playlist playlistHeader;


        class CircleButton : public juce::Component {
        public:
            std::function<void()> onClick;
            void paint(juce::Graphics& g) override {
                g.setColour(isHovered ? juce::Colour(106, 106, 115) : juce::Colour(23, 23, 27));
                g.fillEllipse(getLocalBounds().toFloat());
                g.setColour(juce::Colours::white);
                g.setFont(18.0f);
                g.drawText("+", getLocalBounds(), juce::Justification::centred);
            }
            void mouseDown(const juce::MouseEvent&) override {
                if (onClick) onClick();
            }
            void mouseEnter(const juce::MouseEvent&) override {
                isHovered = true;
                repaint();
            }
            void mouseExit(const juce::MouseEvent&) override {
                isHovered = false;
                repaint();
            }

        private:
            bool isHovered = false;
        };

        CircleButton addPlaylistButton;
        std::unique_ptr<juce::FileChooser> playlistFileChooser;
        // Open multi-select chooser and add selected files to playlist.
        void openFileChooser();
        // Extract metadata/analysis and append one file to playlist.
        void processAndAddFileToPlaylist(const juce::File& file);

        //--- Deck GUIs ---//
        DeckGUI deckLeft {"Left", transportSourceLeft, DeckGUI::DeckSide::Left};
        DeckGUI deckRight {"Right", transportSourceRight, DeckGUI::DeckSide::Right};

        AudioAnalyzer audioAnalyzer;

        // Audio state tracking
        double currentSampleRate {0.0};
        int currentBufferSize {0};
        
        // Volume and crossfader values
        std::atomic<float> leftVolumeGain {0.75f};
        std::atomic<float> rightVolumeGain {0.75f};

        std::atomic<float> crossfaderPosition {0.5f};

        std::atomic<float> leftTrimGain {1.0f};
        std::atomic<float> rightTrimGain {1.0f};


        // EQ values
        std::atomic<float> leftHighEQ {0.0f};
        std::atomic<float> leftMidEQ {0.0f};
        std::atomic<float> leftLowEQ {0.0f};

        std::atomic<float> rightHighEQ {0.0f};
        std::atomic<float> rightMidEQ {0.0f};
        std::atomic<float> rightLowEQ {0.0f};

        // Hot cue persistence
        static constexpr int hotCuePadCount = 8;
        std::map<juce::String, std::vector<double>> hotCueStore;

        // Sampler persistence (per deck, not per song)
        std::array<juce::String, numSamplerPads> leftSamplerStore {};
        std::array<juce::String, numSamplerPads> rightSamplerStore {};

        // Return hot cue persistence file path.
        juce::File getHotCuesFile() const;
        // Load hot cue store from disk.
        void loadHotCuesStore();
        // Save hot cue store to disk.
        void saveHotCuesStore() const;
        // Get persisted hot cues for a specific track.
        std::vector<double> getHotCuesForFile(const juce::File& file) const;
        // Update persisted hot cues for a specific track.
        void updateHotCuesForFile(const juce::File& file,
                        const std::vector<std::pair<double, int>>& markers);

        // Return sampler persistence file path.
        juce::File getSamplerStateFile() const;
        // Load sampler assignments from disk.
        void loadSamplerStateStore();
        // Save sampler assignments to disk.
        void saveSamplerStateStore() const;
        // Return persisted sampler files for one deck.
        std::vector<juce::File> getSamplerFilesForDeck(int deckIndex) const;
        // Persist one sampler pad assignment for one deck.
        void updateSamplerPadForDeck(int deckIndex, int padIndex, const juce::File& file);
        // Clear persisted assignments for one deck.
        void clearSamplerDeck(int deckIndex);

        // Mixer persistence (per track)
        struct MixerState {
            float trim {0.0f};
            float high {0.0f};
            float mid {0.0f};
            float low {0.0f};
            float volume {0.75f};
        };

        struct MixerStateRecord {
            MixerState original;
            MixerState last;
            bool hasOriginal {false};
        };

        std::map<juce::String, MixerStateRecord> mixerStateStore;

        // Return mixer persistence file path.
        juce::File getMixerStateFile() const;
        // Load mixer state records from disk.
        void loadMixerStateStore();
        // Save mixer state records to disk.
        void saveMixerStateStore() const;
        // Get last mixer state for a specific track.
        MixerState getMixerStateForFile(const juce::File& file) const;
        // Update mixer state record for a specific track.
        void updateMixerStateForFile(const juce::File& file, const MixerState& state);


        // EQ filters (3-band per deck)
        juce::IIRFilter leftLowFilter[2];
        juce::IIRFilter leftMidFilter[2];
        juce::IIRFilter leftHighFilter[2];

        juce::IIRFilter rightLowFilter[2];
        juce::IIRFilter rightMidFilter[2];
        juce::IIRFilter rightHighFilter[2];
        
        // Recompute EQ filter coefficients from current control values.
        void updateEQFilters();        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};