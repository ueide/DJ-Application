/*
    DeckGUI.cpp
    Deck UI implementation for transport controls, tempo/sync, looping, cues, and pads.
    Main responsibilities:
    - Handle user interactions with deck controls and update transport state.
    - Provide visual feedback for current track metadata, BPM, loop status, and cue points.
*/


#include <JuceHeader.h>
#include "DeckGUI.h"


//--- Load button LookAndFeel ---//
juce::Font LoadButtonLookAndFeel::getTextButtonFont(juce::TextButton&, int /*buttonHeight*/) {
    return juce::Font(10.0f, juce::Font::plain);
}


//--- Load button background ---//
void LoadButtonLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                    const juce::Colour& backgroundColour,
                                    bool isMouseOverButton, bool ) {
    
    auto buttonArea = button.getLocalBounds().toFloat(); // Button area

    // Change colour on hover
    if (isMouseOverButton) {g.setColour(juce::Colour(42, 42, 44));}
    else {g.setColour(backgroundColour);}

    g.fillRoundedRectangle(buttonArea, 2.0f); //Corners

    // Draw stroke
    g.setColour(juce::Colour(78, 75, 75));
    g.drawRoundedRectangle(buttonArea, 4.0f, 2.0f);
}


//--- Sync button LookAndFeel ---//
void SyncButtonLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool isMouseOverButton, bool isButtonDown) {
    
    auto area = button.getLocalBounds().toFloat();
    auto colour = standardColour;

    // Determine colour based on state
    if (button.getToggleState() || isButtonDown) {colour = activeColour;}
    else if (isMouseOverButton) {colour = hoverColour;}

    g.setColour(colour);
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawRoundedRectangle(area, 8.0f, 1.2f); // Stroke
}


//--- Sync button text ---//
void SyncButtonLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                        bool /*isMouseOverButton*/, bool /*isButtonDown*/) {
    g.setColour(juce::Colour(1, 1, 1)); // RGB(1,1,1)
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}


//--- DeckGUI implementation ---//
DeckGUI::DeckGUI(juce::String deckName, juce::AudioTransportSource& transportToUse, DeckSide side)
                        : name(deckName), transport(transportToUse), deckSide(side) {

    jogWheel.setRange(0.0, 1.0); // range for jog wheel

    //========== LEFT SIDE ELEMENTS ==========//
    
    //--- Load Button (Left) ---//
    addAndMakeVisible(loadButton);
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(26, 26, 28));
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(215, 215, 215));
    loadButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(26, 26, 28));
    loadButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    loadButton.setLookAndFeel(&loadButtonLookAndFeel);

    loadButton.onClick = [this]() {
        auto chooserFlags = juce::FileBrowserComponent::openMode 
                        | juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser = std::make_unique<juce::FileChooser>("Select a music file to play...",
                                                        juce::File{},
                                                        "*.mp3;*.wav;*.aiff");
        
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
            auto file = chooser.getResult();
            if (file != juce::File{} && onFileLoaded) {
                onFileLoaded(file);
            }
        });
    };

    //--- Sync Button (Left) ---//
    addAndMakeVisible(syncButton);
    syncButton.setLookAndFeel(&syncButtonLookAndFeel);
    syncButton.setClickingTogglesState(true);
    syncButton.setColour(juce::TextButton::buttonColourId, juce::Colour(114, 212, 114));
    syncButton.setColour(juce::TextButton::textColourOffId, juce::Colour(1, 1, 1));
    syncButton.setColour(juce::TextButton::textColourOnId, juce::Colour(1, 1, 1));
    syncButton.onClick = [this]() {
        bool newSyncState = syncButton.getToggleState();
        // Only apply sync when turning it ON (false -> true)
        if (newSyncState && !isSyncActive) {
            isSyncActive = true;
            if (onSyncPressed) {
                onSyncPressed();
            }
        } else if (!newSyncState) {
            // When turning sync OFF, reset to tempo slider value
            isSyncActive = false;
            double pct = tempoSlider.getValue();
            baseSpeedRatio = 1.0 + (pct / 100.0);
            baseSpeedRatio = juce::jmax(0.01, baseSpeedRatio);
            if (resamplingSource != nullptr) {
                resamplingSource->setResamplingRatio(baseSpeedRatio);
            }
        }
        repaint();
    };

    //--- Loop Controls (Left) ---//
    addAndMakeVisible(loopLabel);
    loopLabel.setText("Loop", juce::dontSendNotification);
    loopLabel.setFont(juce::Font(12.0f));
    loopLabel.setColour(juce::Label::textColourId, juce::Colour(203, 197, 197));
    loopLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(loopDisplay);
    loopDisplay.setText("4", juce::dontSendNotification);
    loopDisplay.setFont(juce::Font(12.0f));
    loopDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(43, 43, 46));
    loopDisplay.setColour(juce::Label::textColourId, juce::Colour(215, 215, 215));
    loopDisplay.setJustificationType(juce::Justification::centred);

    // Loop arrow icons
    {
        // Left Arrow SVG
        auto leftSvg = juce::parseXML(R"(
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none">
            <path fill-rule="evenodd" clip-rule="evenodd" d="M14.5821 7.04448C14.7594 7.09474 14.8942 7.18263 14.9569 7.28888C15.0196 7.39513 15.005 7.51107 14.9164 7.61127L10.5878 12.4984L14.9179 17.3846C14.9629 17.4343 14.9901 17.4886 14.9977 17.5442C15.0054 17.5999 14.9935 17.6558 14.9627 17.7089C14.9318 17.7619 14.8827 17.811 14.8181 17.8533C14.7534 17.8955 14.6746 17.9302 14.5862 17.9552C14.4977 17.9802 14.4014 17.9951 14.3027 17.999C14.204 18.0029 14.1049 17.9957 14.0111 17.9779C13.9174 17.9601 13.8308 17.932 13.7564 17.8952C13.6819 17.8584 13.6211 17.8137 13.5775 17.7636L9.07944 12.6879C9.0272 12.629 9 12.5641 9 12.4984C9 12.4326 9.0272 12.3677 9.07944 12.3089L13.5775 7.23313C13.6666 7.13306 13.8224 7.05701 14.0107 7.02165C14.199 6.98628 14.4045 6.99449 14.5821 7.04448Z" fill="#141416"/>
            </svg>
            )");
        leftArrowIcon = juce::Drawable::createFromSVG(*leftSvg);
        
        // Right arrow SVG
        auto rightSvg = juce::parseXML(R"(
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none">
            <path fill-rule="evenodd" clip-rule="evenodd" d="M9.41642 7.04482C9.59418 6.99457 9.80003 6.98621 9.9887 7.02159C10.1774 7.05696 10.3334 7.13318 10.4225 7.23346L14.9206 12.309C14.9728 12.3679 15 12.4327 15 12.4985C15 12.5643 14.9728 12.6292 14.9206 12.688L10.4225 17.7636C10.3789 17.8137 10.3181 17.8584 10.2436 17.8952C10.1692 17.932 10.0826 17.9601 9.98886 17.9779C9.8951 17.9957 9.79601 18.0029 9.69731 17.999C9.59862 17.9951 9.50227 17.9802 9.41381 17.9552C9.32536 17.9302 9.24655 17.8955 9.18194 17.8533C9.11733 17.811 9.06819 17.7619 9.03735 17.7089C9.00651 17.6558 8.99458 17.5999 9.00226 17.5442C9.00993 17.4886 9.03706 17.4343 9.08207 17.3846L13.4107 12.4985L9.08207 7.61244C8.993 7.51215 8.97819 7.39601 9.04089 7.28956C9.10359 7.18311 9.23868 7.09508 9.41642 7.04482Z" fill="#141416"/>
            </svg>
            )");
        rightArrowIcon = juce::Drawable::createFromSVG(*rightSvg);
    }

    // Loop arrow buttons
    leftArrowButton = std::make_unique<juce::DrawableButton>("Left", juce::DrawableButton::ImageFitted);
    rightArrowButton = std::make_unique<juce::DrawableButton>("Right", juce::DrawableButton::ImageFitted);

    // Draw controls
    addAndMakeVisible(leftArrowButton.get());
    addAndMakeVisible(rightArrowButton.get());
    leftArrowButton->setLookAndFeel(&arrowButtonLookAndFeel);
    rightArrowButton->setLookAndFeel(&arrowButtonLookAndFeel);

    leftArrowButton->setImages(leftArrowIcon.get(), leftArrowIcon.get(), leftArrowIcon.get(), leftArrowIcon.get());
    rightArrowButton->setImages(rightArrowIcon.get(), rightArrowIcon.get(), rightArrowIcon.get(), rightArrowIcon.get());

    leftArrowButton->setEdgeIndent(6);
    rightArrowButton->setEdgeIndent(6);

    leftArrowButton->onClick = [this]() { cycleLoopBeats(false); };
    rightArrowButton->onClick = [this]() { cycleLoopBeats(true); };

    //--- Loop Toggle Button (Left) ---//
    // Toggle button to activate/deactivate loop
    addAndMakeVisible(loopToggleButton);
    loopToggleButton.setLookAndFeel(&standardToggleLookAndFeel);
    loopToggleButton.setClickingTogglesState(false);
    loopToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(26, 26, 28));
    loopToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(215, 215, 215));
    loopToggleButton.onClick = [this]() {
        if (!isLooping) {
            activateLoop();
            loopToggleButton.setToggleState(true, juce::dontSendNotification);
        } else {
            disableLoop();
            loopToggleButton.setToggleState(false, juce::dontSendNotification);
        }
        repaint();
    };


    //--- Cue Button (Left) ---//
    addAndMakeVisible(cueButton);
    cueButton.setClickingTogglesState(false);
    cueButton.addMouseListener(this, true);
    cueButton.onClick = [this]() {
        const bool hasAudioLoaded = transport.getLengthInSeconds() > 0.0;
        if (!hasAudioLoaded) {
            isCueActive = false;
            hasCuePoint = false;
            cuePreviewing = false;
            repaint();
            return;
        }

        if (!hasCuePoint) {
            const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
            cueSample = static_cast<juce::int64>(transport.getCurrentPosition() * sr);
            hasCuePoint = true;
            isCueActive = true;
        } else {
            const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
            const double cuePosSec = static_cast<double>(cueSample) / sr;
            transport.setPosition(cuePosSec);
            transport.stop();
            isPlaying = false;
        }
        cuePreviewing = false;
        repaint();
    };


    //--- Play/Pause Button (Left) ---//
    addAndMakeVisible(playPauseButton);
    juce::String svgData = R"(
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="8" viewBox="0 0 26 11" fill="none">
        <path d="M15.7844 0.864127L10.2146 10.1365" stroke="#141416" stroke-linecap="round"/>
        <path d="M19.5 1C19.8978 1 20.2794 1.15804 20.5607 1.43934C20.842 1.72064 21 2.10218 21 2.5V8.5C21 8.89782 20.842 9.27936 20.5607 9.56066C20.2794 9.84196 19.8978 10 19.5 10C19.1022 10 18.7206 9.84196 18.4393 9.56066C18.158 9.27936 18 8.89782 18 8.5V2.5C18 2.10218 18.158 1.72064 18.4393 1.43934C18.7206 1.15804 19.1022 1 19.5 1V1ZM24.5 1C24.8978 1 25.2794 1.15804 25.5607 1.43934C25.842 1.72064 26 2.10218 26 2.5V8.5C26 8.89782 25.842 9.27936 25.5607 9.56066C25.2794 9.84196 24.8978 10 24.5 10C24.1022 10 23.7206 9.84196 23.4393 9.56066C23.158 9.27936 23 8.89782 23 8.5V2.5C23 2.10218 23.158 1.72064 23.4393 1.43934C23.7206 1.15804 24.1022 1 24.5 1Z" fill="#141416"/>
        <path d="M0 9.33491V1.71767C0 0.951081 0.827079 0.469568 1.49372 0.848047L8.41849 4.77953C9.10415 5.1688 9.09062 6.16144 8.39462 6.5319L1.46985 10.2177C0.803771 10.5722 0 10.0895 0 9.33491Z" fill="#141416"/>
        </svg>
        )";

    // Play/Pause SVG
    auto svgXml = juce::parseXML(svgData);
    if (svgXml != nullptr) {
        playPauseIcon = juce::Drawable::createFromSVG(*svgXml);
    }
    
    playPauseButton.onClick = [this]() {
        const bool hasAudioLoaded = transport.getLengthInSeconds() > 0.0;
        if (!hasAudioLoaded) {
            isPlaying = false;
            repaint();
            return;
        }

        if (transport.isPlaying()) {
            transport.stop();
            isPlaying = false;
        } else {
            transport.start();
            isPlaying = true;
        }
        repaint();
    };

    if (playPauseIcon != nullptr) {
        playPauseButton.setImages(playPauseIcon.get(), playPauseIcon.get(), playPauseIcon.get(), nullptr);
    }
    playPauseButton.setEdgeIndent(9);


    //--- Jog Wheel (Left) ---//
    addAndMakeVisible(jogWheel);
    jogWheel.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    jogWheel.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    jogWheel.addListener(this);
    jogWheel.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    jogWheel.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    jogWheel.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);


    //--- Performance Pads (Left Only) ---//
    for (int i = 0; i < numPads; ++i) {
        addAndMakeVisible(padButtons[i]);
        padButtons[i].setButtonText(juce::String(i + 1));
        padButtons[i].setClickingTogglesState(false);
        padButtons[i].setLookAndFeel(&standardToggleLookAndFeel);
        padButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colour(215, 215, 215));
        padButtons[i].addMouseListener(this, true);
        
        padButtons[i].onClick = [this, i]() {
            triggerPad(i, juce::ModifierKeys::currentModifiers.isShiftDown());
        };
    }

    //--- Pad Mode Buttons - Hot Cue / Sampler (Left Only) ---//
    addAndMakeVisible(hotCueButton);
    addAndMakeVisible(samplerButton);
    addAndMakeVisible(clearHotCuesButton);
    hotCueButton.setClickingTogglesState(true);
    samplerButton.setClickingTogglesState(true);
    hotCueButton.setLookAndFeel(&standardToggleLookAndFeel);
    samplerButton.setLookAndFeel(&standardToggleLookAndFeel);
    clearHotCuesButton.setLookAndFeel(&standardToggleLookAndFeel);
    hotCueButton.setToggleState(false, juce::dontSendNotification);
    samplerButton.setToggleState(false, juce::dontSendNotification);
    clearHotCuesButton.setClickingTogglesState(false);

    clearHotCuesButton.onClick = [this]() {
        if (currentPadMode == PadMode::HotCue) {
            clearAllHotCues();
        } else if (currentPadMode == PadMode::Sampler) {
            clearAllSamples();
        }
    };
    
    // Hot Cue Button
    hotCueButton.onClick = [this]() {
        bool isActive = hotCueButton.getToggleState();
        
        if (isActive) {
            samplerButton.setToggleState(false, juce::dontSendNotification);
            currentPadMode = PadMode::HotCue;
        } else {
            currentPadMode = PadMode::None;
        }
        
        for (int i = 0; i < numPads; ++i) {
            padButtons[i].setToggleState(currentPadMode == PadMode::HotCue && hasHotCue[i], juce::dontSendNotification);
        }
        repaint();
    };
    
    // Sampler Button
    samplerButton.onClick = [this]() {
        bool isActive = samplerButton.getToggleState();
        
        if (isActive) {
            hotCueButton.setToggleState(false, juce::dontSendNotification);
            currentPadMode = PadMode::Sampler;
        } else {
            currentPadMode = PadMode::None;
        }
        
        for (int i = 0; i < numPads; ++i) {
            padButtons[i].setToggleState(currentPadMode == PadMode::Sampler && hasSample[i], juce::dontSendNotification);
        }
        repaint();
    };


    //========== RIGHT SIDE ELEMENTS ==========//

    //--- Tempo Slider (Right) ---//
    addAndMakeVisible(tempoSlider);
    tempoSlider.setSliderStyle(juce::Slider::LinearVertical);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 50, 20);
    tempoSlider.setRange(-16.0, 16.0, 0.1);
    tempoSlider.setValue(0.0);
    tempoSlider.setDoubleClickReturnValue(true, 0.0);
    tempoSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(43, 43, 46));
    tempoSlider.setColour(juce::Slider::trackColourId, juce::Colour(66, 189, 17));
    tempoSlider.setColour(juce::Slider::thumbColourId, juce::Colour(215, 215, 215));
    tempoSlider.setLookAndFeel(&faderLookAndFeel);
    tempoSlider.addListener(this);

    addAndMakeVisible(tempoPerc);
    tempoPerc.setText("0.0", juce::dontSendNotification);
    tempoPerc.setFont(juce::Font(11.0f));
    tempoPerc.setColour(juce::Label::textColourId, juce::Colour(215, 215, 215));
    tempoPerc.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    tempoPerc.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(tempoLabel);
    tempoLabel.setText("Tempo", juce::dontSendNotification);
    tempoLabel.setFont(juce::Font(12.0f));
    tempoLabel.setColour(juce::Label::textColourId, juce::Colour(215, 215, 215));
    tempoLabel.setJustificationType(juce::Justification::centred);

    //--- Timer for UI updates ---//
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    startTimerHz(30);
}

//--- DeckGUI Destructor ---//
// Clean up LookAndFeel pointers
DeckGUI::~DeckGUI() {
    loadButton.setLookAndFeel(nullptr);
    syncButton.setLookAndFeel(nullptr);
    loopToggleButton.setLookAndFeel(nullptr);
    leftArrowButton->setLookAndFeel(nullptr);
    rightArrowButton->setLookAndFeel(nullptr);
    playPauseButton.setLookAndFeel(nullptr);
    hotCueButton.setLookAndFeel(nullptr);
    samplerButton.setLookAndFeel(nullptr);
    clearHotCuesButton.setLookAndFeel(nullptr);
    for (int i = 0; i < numPads; ++i) {
        padButtons[i].setLookAndFeel(nullptr);
    }
    tempoSlider.setLookAndFeel(nullptr);
}


//--- Metadata update ---//
// Called when a new track is loaded to reset state and update metadata
void DeckGUI::updateMetadata(juce::String title, juce::String artist, double bpm) {
    currentTitle = title.isEmpty() ? "Unknown Title" : title;
    currentArtist = artist.isEmpty() ? "Unknown Artist" : artist;
    currentBPM = bpm;

    // Stop playback
    if (transport.isPlaying()) {
        transport.stop();
    }
    isPlaying = false;
    
    // Reset sync
    isSyncActive = false;
    syncButton.setToggleState(false, juce::dontSendNotification);
    
    // Reset loop
    isLooping = false;
    loopStartSample = 0;
    loopEndSample = 0;
    loopToggleButton.setToggleState(false, juce::dontSendNotification);
    notifyLoopRegionChanged();

    // Reset cue
    hasCuePoint = false;
    isCueActive = false;
    cuePreviewing = false;
    cueSample = 0;
    
    // Reset pads and pad mode
    currentPadMode = PadMode::None;
    hotCueButton.setToggleState(false, juce::dontSendNotification);
    samplerButton.setToggleState(false, juce::dontSendNotification);
    for (int i = 0; i < numPads; ++i) {
        hasHotCue[i] = false;
        hotCueSamples[i] = 0;
        padButtons[i].setToggleState(false, juce::dontSendNotification);
    }
    previewingHotCue = -1;

    // Reset tempo slider and speed
    tempoSlider.setValue(0.0, juce::dontSendNotification);
    tempoSlider.setEnabled(true); // Re-enable tempo slider
    tempoPerc.setText("0.0", juce::dontSendNotification);
    baseSpeedRatio = 1.0;
    if (resamplingSource != nullptr) {
        resamplingSource->setResamplingRatio(baseSpeedRatio);
    }

    repaint(); // Update display
}


//--- File Drop Handling ---//
// Handle files dropped onto the deck
void DeckGUI::filesDropped (const juce::StringArray& files, int x, int y) {
    if(onFileLoaded) onFileLoaded(juce::File(files[0]));
}


//--- Sync button logic ---//
// Adjust playback speed to match target BPM
void DeckGUI::updateSpeedToMatchBPM(double targetBPM)  {
    if (targetBPM > 0.0 && currentBPM > 0.0) {
        double speedRatio = targetBPM / currentBPM;
        baseSpeedRatio = speedRatio;
        if (resamplingSource != nullptr) {
            resamplingSource->setResamplingRatio(baseSpeedRatio);
        }
    }
}


//--- Loop Functions ---//
// Predefined loop beat options
static const std::array<double, 7> kLoopBeatOptions { 0.125, 0.25, 2.0, 4.0, 8.0, 16.0, 32.0 };

// Cycle through loop beat options
void DeckGUI::cycleLoopBeats(bool forward) {
    auto it = std::find(kLoopBeatOptions.begin(), kLoopBeatOptions.end(), currentLoopBeats);
    if (it == kLoopBeatOptions.end()) {
        setLoopBeats(4.0);
        return;
    }
    if (forward && std::next(it) != kLoopBeatOptions.end()) {
        setLoopBeats(*std::next(it));
    } else if (!forward && it != kLoopBeatOptions.begin()) {
        setLoopBeats(*std::prev(it));
    }
}

// Set loop length in beats and update loop region if active
void DeckGUI::setLoopBeats(double beats) {
    currentLoopBeats = beats;
    updateLoopDisplay();

    if (isLooping && currentBPM > 0.0) {
        const double beatDuration = 60.0 / currentBPM;
        const double loopDuration = beatDuration * beats;
        loopEndSample = loopStartSample + static_cast<juce::int64>(loopDuration * sampleRate);
        notifyLoopRegionChanged();
    }
}

// Active loop based on the position and beat selection
void DeckGUI::activateLoop() {
    if (currentBPM <= 0.0 || transport.getLengthInSeconds() <= 0.0) {
        isLooping = false;
        notifyLoopRegionChanged();
        return;
    }

    // Ensure sample rate is valid
    sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    // Set loop start at current position
    loopStartSample = static_cast<juce::int64>(transport.getCurrentPosition() * sampleRate);

    // Calculate loop end based on beat length
    const double beatDuration = 60.0 / currentBPM;
    const double loopDuration = beatDuration * currentLoopBeats;
    loopEndSample = loopStartSample + static_cast<juce::int64>(loopDuration * sampleRate);

    // Clamp to track length
    const juce::int64 maxSample = static_cast<juce::int64>(transport.getLengthInSeconds() * sampleRate);
    if (loopEndSample > maxSample) {
        loopEndSample = maxSample;
    }

    isLooping = true;
    notifyLoopRegionChanged();
    repaint();
}

// Disable looping
void DeckGUI::disableLoop() {
    isLooping = false;
    loopStartSample = 0;
    loopEndSample = 0;
    notifyLoopRegionChanged();
    repaint();
}
//------------------------------------------//


// --- Cue Button and Hot Cue Pad Logic --- //
//Handle mouse down events for cue button and hot cue pads
void DeckGUI::mouseDown(const juce::MouseEvent& event) {

    //--------------------- Cue Button Logic --------------------//
    // Cue button hold-to-preview
    if (event.eventComponent == &cueButton) {
        const bool hasAudioLoaded = transport.getLengthInSeconds() > 0.0;
        if (!hasAudioLoaded) {
            isCueActive = false;
            hasCuePoint = false;
            cuePreviewing = false;
            repaint();
            return;
        }

        const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;

        // Shift + click: replace/clear and set a new cue at current position without playing
        if (event.mods.isShiftDown()) {
            cueSample = static_cast<juce::int64>(transport.getCurrentPosition() * sr);
            hasCuePoint = true;
            isCueActive = true;
            cuePreviewing = false;
            repaint();
            return;
        }

        // If no cue point set yet, set it
        if (!hasCuePoint) {
            cueSample = static_cast<juce::int64>(transport.getCurrentPosition() * sr);
            hasCuePoint = true;
            isCueActive = true;
            cuePreviewing = false;
            repaint();
            return;
        }

        // Preview: jump to cue and play while held
        const double cuePosSec = static_cast<double>(cueSample) / sr;
        transport.setPosition(cuePosSec);
        transport.start();
        isPlaying = true;
        cuePreviewing = true;
        repaint();
    }
    
    //--------------------- CUE Logic --------------------//
    // Hot Cue pad hold-to-preview (only in Hot Cue mode)
    if (currentPadMode == PadMode::HotCue) {
        for (int i = 0; i < numPads; ++i) {
            if (event.eventComponent == &padButtons[i]) {
                const bool hasAudioLoaded = transport.getLengthInSeconds() > 0.0;
                if (!hasAudioLoaded) return;
                
                const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
                
                // Shift + click: replace hot cue
                if (event.mods.isShiftDown()) {
                    if (hasHotCue[i]) {
                        clearHotCue(i);
                    }
                    return;
                }
                
                // If no hot cue point set yet, set it
                if (!hasHotCue[i]) {
                    setHotCue(i);
                    return;
                }
                
                // Preview: jump to hot cue and play while held
                const double hotCuePos = static_cast<double>(hotCueSamples[i]) / sr;
                transport.setPosition(hotCuePos);
                transport.start();
                isPlaying = true;
                previewingHotCue = i;
                repaint();
                return;
            }
        }
    }
}


//--- Handle mouse up events for cue button and hot cue pads ---//
void DeckGUI::mouseUp(const juce::MouseEvent& event) {

    //--- Cue Button Logic ---//
    // Cue button preview release
    if (event.eventComponent == &cueButton) {
        if (cuePreviewing) {
            const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
            const double cuePosSec = static_cast<double>(cueSample) / sr;
            transport.stop();
            transport.setPosition(cuePosSec);
            isPlaying = false;
            cuePreviewing = false;
            repaint();
        }
    }
    
    //--- Hot Cue Pad Logic ---//
    // Hot Cue pad preview release
    for (int i = 0; i < numPads; ++i) {
        if (event.eventComponent == &padButtons[i] && previewingHotCue == i) {
            const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
            const double hotCuePos = static_cast<double>(hotCueSamples[i]) / sr;
            transport.stop();
            transport.setPosition(hotCuePos);
            isPlaying = false;
            previewingHotCue = -1;
            repaint();
            return;
        }
    }
}
//------------------------------------------------------------//


//--- Loop Display Update ---//
// Update the loop display label to show current loop length in beats
void DeckGUI::updateLoopDisplay() {
    // Format beats into readable string 
    auto formatBeats = [](double beats) -> juce::String {
        if (std::abs(beats - 0.125) < 1e-6) return "1/8";
        if (std::abs(beats - 0.25) < 1e-6) return "1/4";
        if (std::abs(beats - 2.0)   < 1e-6) return "2";
        if (std::abs(beats - 4.0) < 1e-6) return "4";
        if (std::abs(beats - 8.0) < 1e-6) return "8";
        if (std::abs(beats - 16.0) < 1e-6) return "16";
        if (std::abs(beats - 32.0) < 1e-6) return "32";
        return juce::String(beats, 2);
    };

    loopDisplay.setText(formatBeats(currentLoopBeats), juce::dontSendNotification);
}

// Notify Loop Region Changed
void DeckGUI::notifyLoopRegionChanged() {
    if (!onLoopRegionChanged) return;

    if (isLooping && sampleRate > 0.0) {
        const double startSeconds = static_cast<double>(loopStartSample) / sampleRate;
        const double endSeconds   = static_cast<double>(loopEndSample)   / sampleRate;
        onLoopRegionChanged(std::make_pair(startSeconds, endSeconds));
    } else {
        onLoopRegionChanged(std::nullopt);
    }
}
//---------------------------------------//


//--- Hot Cue Management ---//
// Notify Hot Cues Changed
void DeckGUI::notifyHotCuesChanged() {
    if (!onHotCuesChanged) return;

    const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    std::vector<std::pair<double, int>> markers;
    markers.reserve(numPads);

    for (int i = 0; i < numPads; ++i) {
        if (hasHotCue[i]) {
            const double timeSec = static_cast<double>(hotCueSamples[i]) / sr;
            markers.emplace_back(timeSec, i);
        }
    }

    onHotCuesChanged(markers);
}
//---------------------------------------//


//--- Clear All Hot Cues ---//
void DeckGUI::clearAllHotCues() {
    for (int i = 0; i < numPads; ++i) {
        hasHotCue[i] = false;
        hotCueSamples[i] = 0;
        padButtons[i].setToggleState(false, juce::dontSendNotification);
    }
    previewingHotCue = -1;
    notifyHotCuesChanged();
    repaint();
}
//---------------------------//


//---------- Slider Listener -----------//
void DeckGUI::sliderValueChanged(juce::Slider* slider) {

    //--- Jog Wheel logic ---//
    if (slider == &jogWheel && isDraggingJog) {
        double jogPos = jogWheel.getValue(); // 0.0 to 1.0
        double songLengthSec = transport.getLengthInSeconds();
        
        // Map jog position to track position
        if (songLengthSec > 0) {
            double newTransportPos = jogPos * songLengthSec;
            transport.setPosition(newTransportPos);
        }
    }


    //--- Tempo Slider logic ---//
    if (slider == &tempoSlider) {
        // If sync is active, disable it when user touches tempo slider
        if (isSyncActive) {
            isSyncActive = false;
            syncButton.setToggleState(false, juce::dontSendNotification);
        }
        
        // Map slider value (-16.0 to +16.0) to speed ratio (0.84x to 1.16x)
        double pct = slider->getValue();
        baseSpeedRatio = 1.0 + (pct / 100.0);
        baseSpeedRatio = juce::jmax(0.01, baseSpeedRatio); // Prevent zero or negative speed

        // Apply new speed
        if (resamplingSource != nullptr) {
            resamplingSource->setResamplingRatio(baseSpeedRatio);
        }

        // Update tempo percentage display
        double pctDisp = slider->getValue();
        juce::String sign = (pctDisp > 0.0 ? "+" : (pctDisp < 0.0 ? "" : ""));
        tempoPerc.setText(sign + juce::String(pctDisp, 1), juce::dontSendNotification);
    }
}


//Jog Wheel Dragging
void DeckGUI::sliderDragStarted(juce::Slider* slider) {
    if (slider == &jogWheel) {
        isDraggingJog = true;
        lastJogValue = jogWheel.getValue();
    }
}


//Jog Wheel Drag Ended 
void DeckGUI::sliderDragEnded(juce::Slider* slider) {
    if (slider == &jogWheel) {
        isDraggingJog = false;
    }
}


//--- Tempo Slider Change ---//
// Handled in sliderValueChanged()
void DeckGUI::timerCallback() {
    if(transport.getLengthInSeconds() > 0) {
        // Check and handle looping (sample-accurate)
        if (isLooping && transport.isPlaying()) {
            juce::int64 currentSample = static_cast<juce::int64>(transport.getCurrentPosition() * sampleRate);
            if (currentSample >= loopEndSample) {
                // Jump back to loop start (sample-accurate)
                transport.setPosition(static_cast<double>(loopStartSample) / sampleRate);
            }
        }
        
        // Update actual playhead position from transport
        double progress = transport.getCurrentPosition() / transport.getLengthInSeconds();
        playheadPosition = progress;
        
        // Always repaint to show progress moving
        repaint();
        
        // Update jog wheel position if not dragging
        if (!isDraggingJog) {
            const double threshold = 0.001;
            double expectedJogVal = transport.getCurrentPosition() / transport.getLengthInSeconds();
            if (std::abs(expectedJogVal - jogWheel.getValue()) > threshold) {
                jogWheel.setValue(expectedJogVal, juce::dontSendNotification);
            }
        }
    }
}


void DeckGUI::paint (juce::Graphics& g) {
    auto area = getLocalBounds();
    const int deckWidth = area.getWidth();

    //------------- Title Box -------------//
    // Title box dimensions
    const int titleBoxWidth = 358;
    const int titleBoxHeight = 56;
    const int titleBoxMargin = 24;
    
    // LEFT DECK
    const int leftTitleBoxX = titleBoxMargin;
    
    // RIGHT DECK 
    const int rightTitleBoxX = deckWidth - titleBoxMargin - titleBoxWidth;
    
    // Select X position based on deck side
    const int titleBoxX = (deckSide == DeckSide::Left) ? leftTitleBoxX : rightTitleBoxX;
    auto titleBox = juce::Rectangle<int>(titleBoxX, 0, titleBoxWidth, titleBoxHeight);
    
    g.setColour(juce::Colour(43, 43, 45)); // background color
    g.fillRoundedRectangle(titleBox.toFloat(), 6.0f);

    // Title area
    auto titleArea = titleBox.withTrimmedLeft(16).withTrimmedTop(8);
    g.setColour(juce::Colour(215, 215, 215));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(currentTitle, titleArea.removeFromTop(20).withWidth(287), juce::Justification::left);

    // Artist area
    auto artistArea = titleBox.withTrimmedLeft(16).withTrimmedTop(16 + 12);
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText(currentArtist, artistArea.removeFromTop(16).withWidth(287), juce::Justification::left);

    // BPM number
    auto bpmNumberArea = titleBox.withTrimmedLeft(16 + 278 + 16).withTrimmedTop(8);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    const bool hasTrackLoaded = (currentTitle != "No Track Loaded");
    juce::String bpmText = (currentBPM > 0) ? juce::String((int)currentBPM) : "0";
    g.drawText(bpmText, bpmNumberArea.withHeight(20), juce::Justification::centred);

    // BPM label
    auto bpmLabelArea = titleBox.withTrimmedLeft(16 + 276 + 16).withTrimmedTop(8 + 16 + 4); 
    g.setFont(12.0f);
    g.drawText("BPM", bpmLabelArea.withHeight(16), juce::Justification::centred);
    //------------------------------------//


    //-------- Play/Pause button --------//
    auto playBounds = playPauseButton.getBounds().toFloat();
    juce::Colour playColour = playPauseButtonColour;

    // Change colour based on state
    if (isPlaying) {
        playColour = playPauseButtonActiveColour;
    } else if (playPauseButton.isMouseOver()) {
        playColour = playPauseButtonHoverColour;
    }

    g.setColour(playColour);
    g.fillEllipse(playBounds);
    //------------------------------------//


    //-------- Cue button --------//
    auto cueBounds = cueButton.getBounds().toFloat();
    juce::Colour cueColour = playPauseButtonColour;

    // Change colour based on state
    if (isCueActive) {
        cueColour = playPauseButtonActiveColour;
    } else if (cueButton.isMouseOver()) {
        cueColour = playPauseButtonHoverColour;
    }

    g.setColour(cueColour);
    g.fillEllipse(cueBounds);
    
    // Draw CUE text
    g.setColour(juce::Colours::black);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("CUE", cueBounds.toNearestInt(), juce::Justification::centred);
    //------------------------------------//


    //-------- Jog Wheel --------//
    // Use the actual component bounds from resized()
    auto wheelBounds = jogWheel.getBounds();
    auto wheelArea = wheelBounds;
    
    // Big circle
    g.setColour(juce::Colour(23, 23, 25));
    g.fillEllipse(wheelArea.toFloat());
    
    // Middle circle
    int middleSize = 172;
    auto middleArea = wheelArea.withSizeKeepingCentre(middleSize, middleSize);
    g.setColour(juce::Colour(32, 32, 35));
    g.fillEllipse(middleArea.toFloat());
    
    // Center circle
    int centerSize = 36;
    auto centerArea = wheelArea.withSizeKeepingCentre(centerSize, centerSize).translated(0, -4);
    
    // Draw center circle
    g.setColour(juce::Colour(23, 23, 25));
    g.fillEllipse(centerArea.reduced(1).toFloat());

    // Draw border
    g.setColour(juce::Colour(20, 20, 23));
    g.drawEllipse(centerArea.toFloat(), 2.0f);

    // Progress ring
    const float lineThickness = 4.0f;
    auto ringArea = wheelArea.expanded(6);
    g.setColour(juce::Colour(49, 50, 53));
    g.drawEllipse(ringArea.toFloat(), lineThickness);

    // Progress arc
    g.setColour(juce::Colour(66, 189, 17));
    juce::Path progressPath;
    progressPath.addCentredArc(
        ringArea.getCentreX(),
        ringArea.getCentreY(),
        ringArea.getWidth() / 2.0f,
        ringArea.getHeight() / 2.0f,
        0.0f,
        0.0f,
        juce::MathConstants<float>::twoPi * playheadPosition,
        true);
    g.strokePath(progressPath, juce::PathStrokeType(lineThickness));

    // Playhead indicator 
    float playheadAngle = juce::MathConstants<float>::twoPi * playheadPosition - juce::MathConstants<float>::halfPi;
    float playheadRadius = ringArea.getWidth() / 2.0f;
    float playheadX = ringArea.getCentreX() + playheadRadius * std::cos(playheadAngle);
    float playheadY = ringArea.getCentreY() + playheadRadius * std::sin(playheadAngle);
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(playheadX - 5.0f, playheadY - 5.0f, 10.0f, 10.0f);
    //------------------------------------//
}


void DeckGUI::resized() {
    auto area = getLocalBounds();
    const int deckWidth = area.getWidth();

    //========== LAYOUT CONSTANTS ==========//
    const int titleBoxWidth = 358;
    const int titleBoxHeight = 56;
    const int edgeMargin = 24;
    const int wheelSize = 188;
    const int arrowSize = 24;
    const int arrowSpacing = 32;

    //========== LEFT DECK LAYOUT ==========//
    
    // LEFT: Control column (Load, Sync, Loop, Cue, Play)
    const int leftControlsX = CONTROLS_COLUMN_X;
    const int leftColumnCenterX = leftControlsX + (CONTROLS_COLUMN_WIDTH / 2);
    
    // LEFT: Sync button
    const int leftSyncX = leftColumnCenterX - (BUTTON_SIZE_48 / 2);
    
    // LEFT: Loop controls
    const int leftLoopStartY = 52 + BUTTON_SIZE_48 + 32;
    const int leftDisplayY = leftLoopStartY + 18;
    const int leftArrowY = leftDisplayY + 32;
    const int leftLoopButtonY = leftArrowY + 36;
    
    // LEFT: Play/Pause and Cue buttons
    const int leftPlayX = leftColumnCenterX - (BUTTON_SIZE_48 / 2);
    const int leftPlayY = 416 - BUTTON_SIZE_48;
    const int leftCueY = leftPlayY - 20 - BUTTON_SIZE_48;
    
    // LEFT: Jog wheel (centered in title box area)
    const int leftWheelX = edgeMargin + (titleBoxWidth - wheelSize) / 2;
    const int leftWheelY = titleBoxHeight + edgeMargin;


    //========== RIGHT DECK LAYOUT ==========//
    
    // RIGHT: Control column (Load, Sync, Loop, Cue, Play)
    const int rightControlsX = deckWidth - CONTROLS_COLUMN_X - CONTROLS_COLUMN_WIDTH;
    const int rightColumnCenterX = rightControlsX + (CONTROLS_COLUMN_WIDTH / 2);
    
    // RIGHT: Sync button
    const int rightSyncX = rightColumnCenterX - (BUTTON_SIZE_48 / 2);
    
    // RIGHT: Loop controls
    const int rightLoopStartY = 52 + BUTTON_SIZE_48 + 32;
    const int rightDisplayY = rightLoopStartY + 18;
    const int rightArrowY = rightDisplayY + 32;
    const int rightLoopButtonY = rightArrowY + 36;
    
    // RIGHT: Play/Pause and Cue buttons
    const int rightPlayX = rightColumnCenterX - (BUTTON_SIZE_48 / 2);
    const int rightPlayY = 416 - BUTTON_SIZE_48;
    const int rightCueY = rightPlayY - 20 - BUTTON_SIZE_48;
    
    // RIGHT: Jog wheel (centered in title box area, from right edge)
    const int rightWheelX = deckWidth - edgeMargin - titleBoxWidth + (titleBoxWidth - wheelSize) / 2;
    const int rightWheelY = titleBoxHeight + edgeMargin;


    //========== APPLY LAYOUT BASED ON DECK SIDE ==========//
    
    //--- Load button ---//
    const int controlsX = (deckSide == DeckSide::Left) ? leftControlsX : rightControlsX;
    loadButton.setBounds(controlsX, 0, CONTROLS_COLUMN_WIDTH, 24);

    //--- Sync button ---//
    const int syncX = (deckSide == DeckSide::Left) ? leftSyncX : rightSyncX;
    syncButton.setBounds(syncX, 52, BUTTON_SIZE_48, BUTTON_SIZE_48);

    //--- Loop controls ---//
    const int loopStartY = (deckSide == DeckSide::Left) ? leftLoopStartY : rightLoopStartY;
    const int displayY = (deckSide == DeckSide::Left) ? leftDisplayY : rightDisplayY;
    const int arrowY = (deckSide == DeckSide::Left) ? leftArrowY : rightArrowY;
    const int loopButtonY = (deckSide == DeckSide::Left) ? leftLoopButtonY : rightLoopButtonY;
    
    loopLabel.setBounds(controlsX, loopStartY, CONTROLS_COLUMN_WIDTH, 16);
    loopDisplay.setBounds(controlsX, displayY, CONTROLS_COLUMN_WIDTH, 24);
    leftArrowButton->setBounds(controlsX, arrowY, arrowSize, arrowSize);
    rightArrowButton->setBounds(controlsX + arrowSpacing, arrowY, arrowSize, arrowSize);
    loopToggleButton.setBounds(controlsX, loopButtonY, CONTROLS_COLUMN_WIDTH, 24);

    //--- Cue and Play/Pause buttons ---//
    const int playX = (deckSide == DeckSide::Left) ? leftPlayX : rightPlayX;
    const int playY = (deckSide == DeckSide::Left) ? leftPlayY : rightPlayY;
    const int cueY = (deckSide == DeckSide::Left) ? leftCueY : rightCueY;
    
    playPauseButton.setBounds(playX, playY, BUTTON_SIZE_48, BUTTON_SIZE_48);
    cueButton.setBounds(playX, cueY, BUTTON_SIZE_48, BUTTON_SIZE_48);

    //--- Jog wheel ---//
    const int wheelX = (deckSide == DeckSide::Left) ? leftWheelX : rightWheelX;
    const int wheelY = (deckSide == DeckSide::Left) ? leftWheelY : rightWheelY;
    jogWheel.setBounds(wheelX, wheelY, wheelSize, wheelSize);

    //--- Performance Pads and Mode Buttons ---//
    const int tempoWidth = 40;
    const int padWidth = 64;
    const int padHeight = 40;
    const int padSpacingH = 16;
    const int padSpacingV = 8;
    const int modeButtonHeight = 24;
    const int modeButtonWidth = 64;
    const int clearButtonWidth = 64;
    
    if (deckSide == DeckSide::Left) {
        // LEFT: Tempo and pad positioning
        const int leftTempoPosX = edgeMargin;
        const int leftTempoPosY = leftWheelY + wheelSize + edgeMargin;
        const int leftTempoWidth = 40;
        const int leftPadStartX = leftTempoPosX + leftTempoWidth + 16;
        const int leftPadStartY = leftTempoPosY;
        
        hotCueButton.setBounds(leftPadStartX, leftPadStartY, modeButtonWidth, modeButtonHeight);
        samplerButton.setBounds(leftPadStartX + modeButtonWidth + padSpacingH, leftPadStartY, modeButtonWidth, modeButtonHeight);

        const int totalPadGridWidth = (padWidth * 4) + (padSpacingH * 3);
        const int clearButtonX = leftPadStartX + totalPadGridWidth - clearButtonWidth;
        clearHotCuesButton.setBounds(clearButtonX, leftPadStartY, clearButtonWidth, modeButtonHeight);

        const int leftGridStartY = leftPadStartY + modeButtonHeight + 12;

        auto placePad = [&](int idx, int col, int row) {
            int x = leftPadStartX + col * (padWidth + padSpacingH);
            int y = leftGridStartY + row * (padHeight + padSpacingV);
            padButtons[idx].setBounds(x, y, padWidth, padHeight);
        };

        placePad(0, 0, 0); // 1
        placePad(2, 1, 0); // 3
        placePad(4, 2, 0); // 5
        placePad(6, 3, 0); // 7

        placePad(1, 0, 1); // 2
        placePad(3, 1, 1); // 4
        placePad(5, 2, 1); // 6
        placePad(7, 3, 1); // 8
    } else {
        // RIGHT: Tempo and pad positioning
        const int rightTempoPosX = deckWidth - edgeMargin - tempoWidth;
        const int rightTempoPosY = rightWheelY + wheelSize + edgeMargin;
        const int rightTempoWidth = 40;
        
        // Calculate total pad grid width (4 columns)
        const int totalPadGridWidth = (padWidth * 4) + (padSpacingH * 3);
        
        // Position pads to the left of tempo slider with 16px spacing
        const int rightPadStartX = rightTempoPosX - 16 - totalPadGridWidth;
        const int rightPadStartY = rightTempoPosY;
        
        hotCueButton.setBounds(rightPadStartX, rightPadStartY, modeButtonWidth, modeButtonHeight);
        samplerButton.setBounds(rightPadStartX + modeButtonWidth + padSpacingH, rightPadStartY, modeButtonWidth, modeButtonHeight);
        clearHotCuesButton.setBounds(rightPadStartX + totalPadGridWidth - clearButtonWidth, rightPadStartY, clearButtonWidth, modeButtonHeight);

        const int rightGridStartY = rightPadStartY + modeButtonHeight + 12;

        auto placePad = [&](int idx, int col, int row) {
            int x = rightPadStartX + col * (padWidth + padSpacingH);
            int y = rightGridStartY + row * (padHeight + padSpacingV);
            padButtons[idx].setBounds(x, y, padWidth, padHeight);
        };

        placePad(0, 0, 0); // 1
        placePad(2, 1, 0); // 3
        placePad(4, 2, 0); // 5
        placePad(6, 3, 0); // 7

        placePad(1, 0, 1); // 2
        placePad(3, 1, 1); // 4
        placePad(5, 2, 1); // 6
        placePad(7, 3, 1); // 8
    }


    //========== TEMPO SLIDER ==========//
    
    const int tempoSliderHeight = 100;
    const int tempoPaddingBottom = 16;
    
    // LEFT: Tempo slider positioning
    const int leftTempoPosX = edgeMargin;
    const int leftTempoPosY = leftWheelY + wheelSize + edgeMargin;
    
    // RIGHT: Tempo slider positioning
    const int rightTempoPosX = deckWidth - edgeMargin - tempoWidth;
    const int rightTempoPosY = rightWheelY + wheelSize + edgeMargin;
    
    // Set tempo slider and labels based on deck side
    if (deckSide == DeckSide::Right) {
        tempoPerc.setBounds(rightTempoPosX, rightTempoPosY, tempoWidth, tempoPaddingBottom);
        tempoSlider.setBounds(rightTempoPosX, rightTempoPosY + 12, tempoWidth, tempoSliderHeight);
        tempoLabel.setBounds(rightTempoPosX, rightTempoPosY + tempoSliderHeight + 8, tempoWidth, tempoPaddingBottom);
    } else {
        tempoPerc.setBounds(leftTempoPosX, leftTempoPosY, tempoWidth, tempoPaddingBottom);
        tempoSlider.setBounds(leftTempoPosX, leftTempoPosY + 12, tempoWidth, tempoSliderHeight);
        tempoLabel.setBounds(leftTempoPosX, leftTempoPosY + tempoSliderHeight + 8, tempoWidth, tempoPaddingBottom);
    }
}


//--- Hot Cue and Sampler Pad Logic ---//
void DeckGUI::setHotCue(int padIndex) {
    if (padIndex < 0 || padIndex >= numPads) return;
    if (transport.getLengthInSeconds() <= 0.0) return;
    
    const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    hotCueSamples[padIndex] = static_cast<juce::int64>(transport.getCurrentPosition() * sr);
    hasHotCue[padIndex] = true;
    // Only show active if in Hot Cue mode
    padButtons[padIndex].setToggleState(currentPadMode == PadMode::HotCue, juce::dontSendNotification);
    notifyHotCuesChanged();
    repaint();
}

// Jump to hot cue and play
void DeckGUI::jumpToHotCue(int padIndex) {
    if (padIndex < 0 || padIndex >= numPads) return;
    if (!hasHotCue[padIndex]) return;
    
    const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    const double hotCuePos = static_cast<double>(hotCueSamples[padIndex]) / sr;
    transport.setPosition(hotCuePos);
    transport.start();
    isPlaying = true;
    repaint();
}

// Clear hot cue from pad
void DeckGUI::clearHotCue(int padIndex) {
    if (padIndex < 0 || padIndex >= numPads) return;
    hasHotCue[padIndex] = false;
    hotCueSamples[padIndex] = 0;
    padButtons[padIndex].setToggleState(false, juce::dontSendNotification);
    notifyHotCuesChanged();
    repaint();
}

// Trigger pad action based on current mode and modifiers
void DeckGUI::triggerPad(int padIndex, bool isShiftDown) {
    if (padIndex < 0 || padIndex >= numPads) return;
    
    if (currentPadMode == PadMode::HotCue) {
        // Hot Cue mode: clear, set, or jump
        if (isShiftDown) {
            // Shift + click: clear existing hot cue
            if (hasHotCue[padIndex]) {
                clearHotCue(padIndex);
            }
        } else if (!hasHotCue[padIndex]) {
            // No hot cue set: create one at current position
            setHotCue(padIndex);
        } else {
            // Has hot cue: jump to it and play
            jumpToHotCue(padIndex);
        }
    } else if (currentPadMode == PadMode::Sampler) {
        // Sampler mode: clear, load, or play sample
        if (isShiftDown) {
            // Shift + click: clear sample from this pad
            clearSample(padIndex);
        } else if (hasSample[padIndex]) {
            // Has sample: trigger playback
            playSample(padIndex);
        } else {
            // Empty pad: choose sample file
            loadSampleToPad(padIndex);
        }
    }
}

//--- Hot Cue Management ---//
// Set hot cues from a list of seconds (called by MainComponent when loading a track)
void DeckGUI::setHotCuesFromSeconds(const std::vector<double>& cueSeconds) {
    // Ensure sample rate is valid
    const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;

    // Update hot cue states based on provided cue times
    for (int i = 0; i < numPads; ++i) {
        const double cueTime = (i < (int)cueSeconds.size()) ? cueSeconds[(size_t)i] : -1.0;
        if (cueTime >= 0.0) {
            hotCueSamples[i] = static_cast<juce::int64>(cueTime * sr);
            hasHotCue[i] = true;
            padButtons[i].setToggleState(currentPadMode == PadMode::HotCue, juce::dontSendNotification);
        } else {
            hasHotCue[i] = false;
            hotCueSamples[i] = 0;
            padButtons[i].setToggleState(false, juce::dontSendNotification);
        }
    }

    previewingHotCue = -1;
    notifyHotCuesChanged();
    repaint();
}

// Load a sample file to the specified pad
void DeckGUI::loadSampleToPad(int padIndex) {
    if (padIndex < 0 || padIndex >= numPads) return;
    
    auto chooserFlags = juce::FileBrowserComponent::openMode 
                    | juce::FileBrowserComponent::canSelectFiles;
    
    // Create and launch file chooser (max 6 seconds enforced in MainComponent)
    sampleFileChooser = std::make_unique<juce::FileChooser>(
        "Select a sample (6 seconds)...",
        juce::File{},
        "*.mp3;*.wav;*.aiff;*.flac");
    
    // Launch asynchronously
    sampleFileChooser->launchAsync(chooserFlags, [this, padIndex](const juce::FileChooser& chooser) {
        auto file = chooser.getResult();
        if (file != juce::File{}) {
            sampleFiles[padIndex] = file;
            hasSample[padIndex] = true;
            
            // Update pad visual state only in Sampler mode
            if (currentPadMode == PadMode::Sampler) {
                padButtons[padIndex].setToggleState(true, juce::dontSendNotification);
            }
            
            // Notify MainComponent to load the sample
            if (onSampleLoaded) {
                onSampleLoaded(padIndex, file);
            }
            repaint();
        }
    });
}

// Restore sampler pads from persisted files
void DeckGUI::setSamplerPadsFromFiles(const std::vector<juce::File>& files) {
    for (int i = 0; i < numPads; ++i) {
        const juce::File file = (i < (int) files.size()) ? files[(size_t) i] : juce::File();
        if (file != juce::File{} && file.existsAsFile()) {
            sampleFiles[i] = file;
            hasSample[i] = true;

            if (onSampleLoaded) {
                onSampleLoaded(i, file);
            }
        } else {
            sampleFiles[i] = juce::File();
            hasSample[i] = false;

            if (onSampleCleared) {
                onSampleCleared(i);
            }
        }

        padButtons[i].setToggleState(currentPadMode == PadMode::Sampler && hasSample[i], juce::dontSendNotification);
    }

    repaint();
}

// Clear sample from a single sampler pad
void DeckGUI::clearSample(int padIndex, bool notifyHost) {
    if (padIndex < 0 || padIndex >= numPads) return;

    sampleFiles[padIndex] = juce::File();
    hasSample[padIndex] = false;
    padButtons[padIndex].setToggleState(false, juce::dontSendNotification);

    if (notifyHost && onSampleCleared) {
        onSampleCleared(padIndex);
    }

    repaint();
}

// Clear all sampler pads
void DeckGUI::clearAllSamples(bool notifyHost) {
    for (int i = 0; i < numPads; ++i) {
        clearSample(i, false);
    }

    if (notifyHost && onAllSamplesCleared) {
        onAllSamplesCleared();
    }

    repaint();
}

// Play a sample assigned to the specified pad
void DeckGUI::playSample(int padIndex) {
    if (padIndex < 0 || padIndex >= numPads) return;
    if (!hasSample[padIndex]) return;
    
    // Notify MainComponent to play the sample
    if (onSampleTrigger) {
        onSampleTrigger(padIndex);
    }
}

// Keyboard handler for pad shortcuts (1-8 keys)
bool DeckGUI::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/) {
    // Map keyboard 1-8 to pads 0-7
    if (key.getKeyCode() >= '1' && key.getKeyCode() <= '8') {
        int padIndex = key.getKeyCode() - '1';
        triggerPad(padIndex, key.getModifiers().isShiftDown());
        return true;
    }
    return false;
}