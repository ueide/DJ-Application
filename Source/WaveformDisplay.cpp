/*
    WaveformDisplay.cpp
    Implements waveform rendering, beat markers, loop overlays, and seeking.
    Main responsibilities:
    - Load audio from URL and generate thumbnail.
    - Render waveform with beat markers (white ticks) and hot cue markers (blue rectangles).
    - Highlight loop regions with a yellow overlay.
    - Handle user interaction for seeking (click/drag) and zooming (arrow keys).
    - Update playhead position in real-time as track plays.
    - Manage resources and repaint efficiently for smooth performance.
*/


#include <JuceHeader.h>
#include "WaveformDisplay.h"

// Constructor
WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& formatManagerToUse,
                                juce::AudioThumbnailCache& thumbnailCacheToUse,
                                juce::String label) 
                            : thumbnail(1000, formatManagerToUse, thumbnailCacheToUse),
                                deckLabel(label) {
    thumbnail.addChangeListener(this);
    
    // Enable keyboard focus to receive arrow key events
    setWantsKeyboardFocus(true);
}


WaveformDisplay::~WaveformDisplay() {
    thumbnail.removeChangeListener(this);
}


//--- Load audio from URL and set as source ---//
void WaveformDisplay::setSource(const juce::URL& audioURL) {
    auto source = std::make_unique<juce::URLInputSource>(audioURL);

    if(thumbnail.setSource(source.release())) {
        fileLoaded = true;
        trackLengthSeconds = thumbnail.getTotalLength();
        playheadPositionSeconds = 0.0;
        repaint();
    } else {
        fileLoaded = false;
        trackLengthSeconds = 0.0;
        playheadPositionSeconds = 0.0;
    }
}


//--- Set beat positions for beat grid ---//
void WaveformDisplay::setBeatPositions(const std::vector<double>& beats) {
    beatPositions = beats;
    repaint();
}


//--- Set hot cue markers ---//
void WaveformDisplay::setHotCueMarkers(const std::vector<std::pair<double, int>>& markers) {
    hotCueMarkers = markers;
    repaint();
}


//--- Set loop region (in seconds) ---//
void WaveformDisplay::setLoopRegion(std::optional<std::pair<double, double>> loopRegion) {
    // Validate loop region
    if (loopRegion && loopRegion->second <= loopRegion->first) {
        loopRegionSeconds.reset();
    } else {
        loopRegionSeconds = loopRegion;
    }
    repaint();
}


//--- Draw the waveform when the thumbnail changes ---//
void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* source) {
    trackLengthSeconds = thumbnail.getTotalLength();
    repaint();
}


void WaveformDisplay::paint (juce::Graphics& g) {
    auto area = getLocalBounds();

    // Background color
    g.fillAll (juce::Colour(28, 28, 29));

    //Label box (left side)
    auto labelArea = area.removeFromLeft(64);
    // Label area with 50% opacity
    g.setColour(juce::Colour(46, 46, 50).withAlpha(0.5f));
    g.fillRect(labelArea);

    // Label text color
    g.setColour(juce::Colour(215, 215, 215));
    g.setFont(12.0f);
    g.drawText(deckLabel, labelArea, juce::Justification::centred);

    // --- Draw Waveform --- //
    if (fileLoaded) {
        // Calculate waveform area
        auto waveformArea = area.withTrimmedTop(4).withTrimmedBottom(4);

        const double total = trackLengthSeconds > 0.0 ? trackLengthSeconds : thumbnail.getTotalLength();
        
        // Calculate effective window with horizontal zoom
        const double effectiveWindow = getEffectiveWindowSeconds();
        
        // Calculate pixels per second and total display width
        const double pixelsPerSecond = (double)waveformArea.getWidth() / effectiveWindow;
        const double totalDisplayWidth = total * pixelsPerSecond;
        
        // Calculate playhead pixel position and offset
        const double playheadPixelPos = playheadPositionSeconds * pixelsPerSecond;
        const double centerPixel = (double)getLocalBounds().getCentreX() - (double)waveformArea.getX();
        const double pixelOffset = centerPixel - playheadPixelPos;

        // Calculate visible time range
        double visibleStartTime = playheadPositionSeconds - (effectiveWindow * 0.5);
        double visibleEndTime = visibleStartTime + effectiveWindow;
        visibleStartTime = juce::jmax(0.0, visibleStartTime);
        visibleEndTime = juce::jmin(total, visibleEndTime);

        {
            juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(waveformArea);
            
            // Draw waveform in lime green
            g.setColour(juce::Colours::limegreen);
            juce::Rectangle<int> scaledArea(waveformArea.getX() + (int)pixelOffset, 
                                            waveformArea.getY(), 
                                            (int)totalDisplayWidth, 
                                            waveformArea.getHeight());

            // Vertical zoom factor
            float verticalZoom = (float)verticalZoomLevel;
            thumbnail.drawChannel(g, scaledArea, 0.0, total, 0, verticalZoom);
            
            // Draw beat markers
            if (!beatPositions.empty()) {
                g.setColour(juce::Colours::white.withAlpha(0.6f));
                const float tickTop = (float)waveformArea.getY();
                const float tickBottom = tickTop + 12.0f; // 12px height
                for (double beatTime : beatPositions) {
                    if (beatTime < visibleStartTime || beatTime > visibleEndTime)
                        continue;
                    const float beatX = waveformArea.getX() + (float)(pixelOffset + (beatTime * pixelsPerSecond));
                    g.drawVerticalLine((int)beatX, tickTop, tickBottom);
                }
            }

            // Draw hot cue markers (blue rectangles with pad number)
            if (!hotCueMarkers.empty()) {
                const float markerWidth = 12.0f;
                const float markerHeight = 20.0f;
                const float markerY = (float)waveformArea.getY() + 2.0f;

                g.setFont(10.0f);

                for (const auto& marker : hotCueMarkers) {
                    const double timeSec = marker.first;
                    const int padIndex = marker.second;

                    if (timeSec < visibleStartTime || timeSec > visibleEndTime) {
                        continue;
                    }

                    const float markerCenterX = waveformArea.getX()
                        + (float)(pixelOffset + (timeSec * pixelsPerSecond));

                    float markerX = markerCenterX - (markerWidth * 0.5f);
                    markerX = juce::jlimit((float)waveformArea.getX(),
                                        (float)waveformArea.getRight() - markerWidth,
                                        markerX);

                    juce::Rectangle<float> markerRect(markerX, markerY, markerWidth, markerHeight);

                    g.setColour(juce::Colour(70, 140, 255));
                    g.fillRect(markerRect);

                    g.setColour(juce::Colours::white);
                    g.drawText(juce::String(padIndex + 1),
                                markerRect.toNearestInt(),
                                juce::Justification::centred);
                }
            }

            // Highlight loop region -> yellow overlay
            if (loopRegionSeconds && loopRegionSeconds->second > loopRegionSeconds->first) {
                const double loopStart = juce::jlimit(0.0, total, loopRegionSeconds->first);
                const double loopEnd   = juce::jlimit(0.0, total, loopRegionSeconds->second);

                const double clampedStart = juce::jmax(visibleStartTime, loopStart);
                const double clampedEnd   = juce::jmin(visibleEndTime, loopEnd);

                if (clampedEnd > clampedStart) {
                    const float startX = waveformArea.getX() + (float)(pixelOffset + (clampedStart * pixelsPerSecond));
                    const float endX   = waveformArea.getX() + (float)(pixelOffset + (clampedEnd   * pixelsPerSecond));

                    const float drawStartX = juce::jlimit((float)waveformArea.getX(), (float)waveformArea.getRight(), startX);
                    const float drawEndX   = juce::jlimit((float)waveformArea.getX(), (float)waveformArea.getRight(), endX);
                    const float width      = juce::jmax(1.0f, drawEndX - drawStartX);

                    juce::Rectangle<float> loopRect(drawStartX,
                                                    (float)waveformArea.getY(),
                                                    width,
                                                    (float)waveformArea.getHeight());

                    g.setColour(juce::Colour(211, 177, 71).withAlpha(0.25f));
                    g.fillRect(loopRect);

                    g.setColour(juce::Colour(211, 177, 71).withAlpha(0.9f));
                    g.drawRect(loopRect, 1.2f);
                    g.drawVerticalLine((int)drawStartX, (float)waveformArea.getY(), (float)waveformArea.getBottom());
                }
            }
        }

        // Draw center playhead line
        g.setColour(juce::Colour(255, 255, 255));
        const float centreX = (float)getLocalBounds().getCentreX();
        g.drawVerticalLine(centreX, (float)waveformArea.getY(), (float)waveformArea.getBottom());

    } else {
        auto waveformArea = area.withTrimmedTop(4).withTrimmedBottom(4);
        g.setColour(juce::Colour(215, 215, 215));
        const float centreX = (float)getLocalBounds().getCentreX();
        g.drawVerticalLine(centreX, (float)waveformArea.getY(), (float)waveformArea.getBottom());
    }


    // Draw separator line at the bottom across full width
    g.setColour(juce::Colour(46, 46, 50));
    auto fullArea = getLocalBounds();
    g.drawLine(0.0f, (float)fullArea.getBottom() - 1.0f, (float)fullArea.getRight(), (float)fullArea.getBottom() - 1.0f, 1.0f);
}


//--- Update playback position and track length ---//
void WaveformDisplay::updatePlaybackPosition(double positionSeconds, double trackLengthSecondsIn) {
    trackLengthSeconds = trackLengthSecondsIn;
    const double total = trackLengthSeconds > 0.0 ? trackLengthSeconds : thumbnail.getTotalLength();
    if (total > 0.0)
        playheadPositionSeconds = juce::jlimit(0.0, total, positionSeconds);
    else
        playheadPositionSeconds = juce::jmax(0.0, positionSeconds);
    
    // Repaint every frame for smooth scrolling
    repaint();
}


//--- Set visible time window in seconds ---//
void WaveformDisplay::setWindowSeconds(double seconds) {
    baseWindowSeconds = juce::jmax(1.0, seconds);
    repaint();
}


//--- Get effective window seconds based on horizontal zoom ---//
double WaveformDisplay::getEffectiveWindowSeconds() const {
    // Horizontal zoom levels: 1→2x, 2→3x, 3→4x, 4→5x
    // Using divisors [2,3,4,5] mapped from levels 1-4
    static const std::array<int, 4> kZoomDivisors { 2, 3, 4, 5 };
    const int index = juce::jlimit(0, (int)kZoomDivisors.size() - 1, horizontalZoomLevel - 1);
    return baseWindowSeconds / (double)kZoomDivisors[(size_t)index];
}


//--- Handle keyboard input for zooming ---//
bool WaveformDisplay::keyPressed(const juce::KeyPress& key) {
    //-- verticalZoomLevel
    // Up arrow: increase vertical zoom
    if (key.isKeyCode(juce::KeyPress::upKey)) {
        verticalZoomLevel = juce::jmin(3, verticalZoomLevel + 1);
        repaint();
        return true;
    }

    // Down arrow: decrease vertical zoom
    if (key.isKeyCode(juce::KeyPress::downKey)) {
        verticalZoomLevel = juce::jmax(1, verticalZoomLevel - 1);
        repaint();
        return true;
    }

    //-- horizontalZoomLevel
    // Right arrow: increase horizontal zoom
    if (key.isKeyCode(juce::KeyPress::rightKey)) {
        horizontalZoomLevel = juce::jmin(4, horizontalZoomLevel + 1);
        repaint();
        return true;
    }
    // Left arrow: decrease horizontal zoom
    if (key.isKeyCode(juce::KeyPress::leftKey)) {
        horizontalZoomLevel = juce::jmax(1, horizontalZoomLevel - 1);
        repaint();
        return true;
    }
    return false;
}


//--- Convert pixel X position to time in seconds ---//
// Used for seeking when user clicks/drags on waveform
double WaveformDisplay::pixelToTime(int pixelX) {
    // Get waveform area (after label removal)
    auto area = getLocalBounds();
    area.removeFromLeft(64);
    auto waveformArea = area.withTrimmedTop(4).withTrimmedBottom(4);

    // Calculate current visible window using effective window (with zoom)
    const double total = trackLengthSeconds > 0.0 ? trackLengthSeconds : thumbnail.getTotalLength();
    const double effectiveWindow = getEffectiveWindowSeconds();
    const double visibleDuration = juce::jlimit(1.0, total > 0.0 ? total : effectiveWindow, effectiveWindow);

    double startTime = playheadPositionSeconds - (visibleDuration * 0.5);
    if (total > 0.0) {
        const double maxStart = juce::jmax(0.0, total - visibleDuration);
        startTime = juce::jlimit(0.0, maxStart, startTime);
    } else {
        startTime = juce::jmax(0.0, startTime);
    }

    // Convert pixel position to time
    const double secondsPerPixel = visibleDuration / (double)waveformArea.getWidth();
    const double pixelInArea = pixelX - waveformArea.getX();
    const double timeFromArea = (pixelInArea * secondsPerPixel) + startTime;

    // Clamp to valid range
    return juce::jlimit(0.0, total, timeFromArea);
}

//--- Handle mouse down event for seeking ---//
void WaveformDisplay::mouseDown(const juce::MouseEvent& event) {
    // Grab keyboard focus so arrow keys work
    grabKeyboardFocus();
    
    isDragging = true;
    double seekTime = pixelToTime(event.x);
    if (onSeek) onSeek(seekTime);
}

//--- Handle mouse drag event for seeking ---//
void WaveformDisplay::mouseDrag(const juce::MouseEvent& event) {
    if (isDragging) {
        double seekTime = pixelToTime(event.x);
        if (onSeek) onSeek(seekTime);
    }
}

void WaveformDisplay::resized() {}