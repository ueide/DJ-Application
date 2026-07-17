/*
    WaveformDisplay.h
    Waveform view with beat grid, hot cues, loop regions, and seeking.
*/


#pragma once
#include <JuceHeader.h>
#include <optional>
#include <utility>
#include "AudioAnalyzer.h"

//--- WaveformDisplay class ---//
class WaveformDisplay  : public juce::Component,
                        public juce::ChangeListener {

    public:
        // Purpose: Create a waveform display for a deck.
        // Inputs: formatManager, thumbnailCache, deck label text.
        // Outputs: initialized WaveformDisplay instance.
        WaveformDisplay(juce::AudioFormatManager& formatManagerToUse,
                        juce::AudioThumbnailCache& thumbnailCacheToUse,
                        juce::String label);
        
        // Purpose: Release waveform resources.
        // Inputs: none.
        // Outputs: none.
        ~WaveformDisplay() override;

        // Purpose: Render the waveform and overlays.
        // Inputs: graphics context.
        // Outputs: none.
        void paint (juce::Graphics&) override;
        // Purpose: Layout waveform bounds.
        // Inputs: none.
        // Outputs: none.
        void resized() override;

        // Purpose: Load audio from URL and update the thumbnail.
        // Inputs: audio URL.
        // Outputs: none.
        void setSource(const juce::URL& audioURL);
        // Purpose: Handle thumbnail change updates.
        // Inputs: change source.
        // Outputs: none.
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        // Purpose: Set beat grid data from analysis.
        // Inputs: beat positions in seconds.
        // Outputs: none.
        void setBeatPositions(const std::vector<double>& beats);

        // Purpose: Set hot cue markers for rendering.
        // Inputs: marker list (time in seconds, pad index).
        // Outputs: none.
        void setHotCueMarkers(const std::vector<std::pair<double, int>>& markers);

        // Purpose: Update playback position and track length.
        // Inputs: position seconds, track length seconds.
        // Outputs: none.
        void updatePlaybackPosition(double positionSeconds, double trackLengthSeconds);

        // Purpose: Set visible time window in seconds.
        // Inputs: window seconds.
        // Outputs: none.
        void setWindowSeconds(double seconds);

        // Purpose: Set loop region (in seconds); pass std::nullopt to clear.
        // Inputs: optional loop region (start, end) in seconds.
        // Outputs: none.
        void setLoopRegion(std::optional<std::pair<double, double>> loopRegionSeconds);

        // Callback when user seeks (clicks/drags waveform); receives time in seconds
        std::function<void(double)> onSeek;

        // Purpose: Handle waveform mouse down for seeking.
        // Inputs: mouse event.
        // Outputs: none.
        void mouseDown(const juce::MouseEvent& event) override;
        // Purpose: Handle waveform mouse drag for scrubbing.
        // Inputs: mouse event.
        // Outputs: none.
        void mouseDrag(const juce::MouseEvent& event) override;
        // Purpose: Handle key presses for waveform interaction.
        // Inputs: key press.
        // Outputs: true if handled.
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        // AudioThumbnail for waveform rendering
        juce::AudioThumbnail thumbnail;
        juce::String deckLabel;
        bool fileLoaded = false;

        double playheadPositionSeconds {0.0};
        double trackLengthSeconds {0.0};
        double baseWindowSeconds {15.0};  // Base window (at zoom x1)
        bool isDragging {false};
        
        // Zoom levels
        // verticalZoomLevel: 1 (normal), 2 (half amp), 3 (quarter amp)
        // horizontalZoomLevel: 1 (5x), 2 (2.5x), 3 (1.67x), 4 (1.25x), 5 (1x)
        int verticalZoomLevel {1};
        int horizontalZoomLevel {1};
        
        // Beat grid data
        std::vector<double> beatPositions;

        // Hot cue markers (time in seconds, pad index)
        std::vector<std::pair<double, int>> hotCueMarkers;

        // Loop region in seconds (start, end)
        std::optional<std::pair<double, double>> loopRegionSeconds;

        // Convert pixel to time based on visible window.
        double pixelToTime(int pixelX);
        
        // Get effective window seconds based on horizontal zoom.
        double getEffectiveWindowSeconds() const;


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};