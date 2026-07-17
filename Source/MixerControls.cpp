/*
    MixerControls.cpp
    This file implements the MixerControls class, which provides a comprehensive set of controls for a DJ mixer interface,
    including trim, EQ, volume sliders, and a crossfader. The class is designed to be visually appealing and user-friendly,
    with a consistent layout and intuitive control scheme. Each control is equipped with callbacks to notify external
    code of changes, allowing for seamless integration with the audio processing backend.
*/


#include <JuceHeader.h>
#include "MixerControls.h"
#include "FaderLookAndFeel.h"


//--- MixerControls visual constants ---//
namespace MixerConstants {
    constexpr float CORNER_RADIUS = 12.0f;
    constexpr float STROKE_WIDTH = 3.0f;

    // Generic knob layout constants (reusable for all knobs)
    constexpr float KNOB_LABEL_FONT_SIZE = 12.0f;
    constexpr float KNOB_SIGN_FONT_SIZE = 12.0f;
    constexpr int KNOB_MARGIN_TOP = 16;
    constexpr int KNOB_MARGIN_SIDE = 16;
    constexpr int KNOB_BOUNDS_SIZE = 64;
    constexpr int KNOB_LABEL_HEIGHT = 18;
    constexpr int KNOB_LABEL_TO_ARC_SPACING = 4;
    constexpr int KNOB_SIGN_WIDTH = 16;
    constexpr int KNOB_SIGN_HEIGHT = 16;
    constexpr int KNOB_SIGN_SPACING = 4;

    // Volume slider constants
    constexpr int VOLUME_SLIDER_WIDTH = 32;
    constexpr int VOLUME_SLIDER_HEIGHT = 82;
    constexpr int VOLUME_LABEL_TOP_GAP = -8;
    constexpr float VOLUME_LABEL_FONT_SIZE = 12.0f;

    // Crossfader slider constants
    constexpr int CROSSFADER_SLIDER_HEIGHT = 40;
    constexpr int CROSSFADER_LABEL_TOP_GAP = -2;
    constexpr float CROSSFADER_LABEL_FONT_SIZE = 12.0f;

    // Knob visual constants (drawn appearance)
    constexpr float KNOB_OUTER_DIAMETER = 32.0f;
    constexpr float KNOB_INNER_DIAMETER = 24.0f;
    constexpr float KNOB_INDICATOR_STROKE = 0.5f;
    constexpr float KNOB_PROGRESS_STROKE = 3.0f;

    // Colour definitions
    const juce::Colour BACKGROUND_COLOUR = juce::Colour(45, 46, 47);
    const juce::Colour STROKE_COLOUR = juce::Colour(42, 43, 44);
    const juce::Colour TEXT_COLOUR = juce::Colour(215, 215, 215);
    const juce::Colour KNOB_OUTER_COLOUR = juce::Colour(28, 28, 32);
    const juce::Colour KNOB_INNER_COLOUR = juce::Colour(33, 33, 38);
    const juce::Colour KNOB_INDICATOR_COLOUR = juce::Colour(215, 215, 215);
    const juce::Colour KNOB_PROGRESS_COLOUR = juce::Colour(100, 200, 100);
    const juce::Colour KNOB_TRACK_COLOUR = juce::Colour(39, 39, 43);
}


// Initializes the mixer controls, including sliders, labels, and callbacks.
MixerControls::MixerControls() {

    // Slider configuration lambdas
    auto configureKnobSlider = [](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange(-1.0, 1.0, 0.2); // 20% steps
        slider.setValue(0.0, juce::dontSendNotification);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f,
                                    true);
    };

    // Volume slider configuration
    auto configureVolumeSlider = [](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange(0.0, 1.5, 0.15); // 15% steps, up to 150%
        slider.setValue(0.75, juce::dontSendNotification); // start at 75%
    };

    // Label configuration
    auto configureLabel = [](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, MixerConstants::TEXT_COLOUR);
        label.setFont(MixerConstants::KNOB_LABEL_FONT_SIZE);
    };

    // Trim Knobs
    configureKnobSlider(leftTrimSlider);
    configureKnobSlider(rightTrimSlider);
    leftTrimSlider.setLookAndFeel(&lookAndFeel);
    rightTrimSlider.setLookAndFeel(&lookAndFeel);

    // Trim listeners
    leftTrimSlider.onValueChange = [this]() {
        leftTrimValue = static_cast<float>(leftTrimSlider.getValue());
        if (onLeftTrimChanged)
            onLeftTrimChanged(leftTrimValue);
    };
    rightTrimSlider.onValueChange = [this]() {
        rightTrimValue = static_cast<float>(rightTrimSlider.getValue());
        if (onRightTrimChanged)
            onRightTrimChanged(rightTrimValue);
    };

    // Volume Sliders
    configureVolumeSlider(leftVolumeSlider);
    configureVolumeSlider(rightVolumeSlider);
    leftVolumeSlider.setLookAndFeel(&lookAndFeel);
    rightVolumeSlider.setLookAndFeel(&lookAndFeel);
    
    // Add volume slider listeners
    leftVolumeSlider.onValueChange = [this]() {
        leftVolumeGain = static_cast<float>(leftVolumeSlider.getValue());
        if (onLeftVolumeChanged)
            onLeftVolumeChanged(leftVolumeGain);
    };
    rightVolumeSlider.onValueChange = [this]() {
        rightVolumeGain = static_cast<float>(rightVolumeSlider.getValue());
        if (onRightVolumeChanged)
            onRightVolumeChanged(rightVolumeGain);
    };

    // Crossfader Slider
    crossfaderSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    crossfaderSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossfaderSlider.setRange(0.0, 1.0, 0.01); // Smooth crossfading
    crossfaderSlider.setValue(0.5, juce::dontSendNotification); // start at center
    crossfaderSlider.setLookAndFeel(&lookAndFeel);
    
    // Add crossfader listener
    crossfaderSlider.onValueChange = [this]() {
        crossfaderPosition = static_cast<float>(crossfaderSlider.getValue());
        if (onCrossfaderChanged)
            onCrossfaderChanged(crossfaderPosition);
    };

    // High Knobs
    configureKnobSlider(leftHighSlider);
    configureKnobSlider(rightHighSlider);
    leftHighSlider.setLookAndFeel(&lookAndFeel);
    rightHighSlider.setLookAndFeel(&lookAndFeel);

    // Mid Knobs
    configureKnobSlider(leftMidSlider);
    configureKnobSlider(rightMidSlider);
    leftMidSlider.setLookAndFeel(&lookAndFeel);
    rightMidSlider.setLookAndFeel(&lookAndFeel);

    // Low Knobs
    configureKnobSlider(leftLowSlider);
    configureKnobSlider(rightLowSlider);
    leftLowSlider.setLookAndFeel(&lookAndFeel);
    rightLowSlider.setLookAndFeel(&lookAndFeel);
    
    // Add EQ listeners for left deck
    auto updateLeftEQ = [this]() {
        leftHighEQ = static_cast<float>(leftHighSlider.getValue());
        leftMidEQ = static_cast<float>(leftMidSlider.getValue());
        leftLowEQ = static_cast<float>(leftLowSlider.getValue());
        if (onLeftEQChanged)
            onLeftEQChanged(leftHighEQ, leftMidEQ, leftLowEQ);
    };
    leftHighSlider.onValueChange = updateLeftEQ;
    leftMidSlider.onValueChange = updateLeftEQ;
    leftLowSlider.onValueChange = updateLeftEQ;
    
    // Add EQ listeners for right deck
    auto updateRightEQ = [this]() {
        rightHighEQ = static_cast<float>(rightHighSlider.getValue());
        rightMidEQ = static_cast<float>(rightMidSlider.getValue());
        rightLowEQ = static_cast<float>(rightLowSlider.getValue());
        if (onRightEQChanged)
            onRightEQChanged(rightHighEQ, rightMidEQ, rightLowEQ);
    };
    rightHighSlider.onValueChange = updateRightEQ;
    rightMidSlider.onValueChange = updateRightEQ;
    rightLowSlider.onValueChange = updateRightEQ;

    // Trim Labels
    configureLabel(leftTrimLabel, "Trim");
    configureLabel(rightTrimLabel, "Trim");

    // High Labels
    configureLabel(leftHighLabel, "High");
    configureLabel(rightHighLabel, "High");

    // Mid Labels
    configureLabel(leftMidLabel, "Mid");
    configureLabel(rightMidLabel, "Mid");

    // Low Labels
    configureLabel(leftLowLabel, "Low");
    configureLabel(rightLowLabel, "Low");

    // Volume Labels
    configureLabel(leftVolumeLabel, "Vol");
    configureLabel(rightVolumeLabel, "Vol");
    leftVolumeLabel.setFont(MixerConstants::VOLUME_LABEL_FONT_SIZE);
    rightVolumeLabel.setFont(MixerConstants::VOLUME_LABEL_FONT_SIZE);

    // Crossfader Label
    configureLabel(crossfaderLabel, "Crossfader");
    crossfaderLabel.setFont(MixerConstants::CROSSFADER_LABEL_FONT_SIZE);

    // Trim +/- Labels
    configureLabel(leftTrimMinusLabel, "-");
    configureLabel(leftTrimPlusLabel, "+");
    configureLabel(rightTrimMinusLabel, "-");
    configureLabel(rightTrimPlusLabel, "+");
    leftTrimMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    leftTrimPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightTrimMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightTrimPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);

    // High +/- Labels
    configureLabel(leftHighMinusLabel, "-");
    configureLabel(leftHighPlusLabel, "+");
    configureLabel(rightHighMinusLabel, "-");
    configureLabel(rightHighPlusLabel, "+");
    leftHighMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    leftHighPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightHighMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightHighPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);

    // Mid +/- Labels
    configureLabel(leftMidMinusLabel, "-");
    configureLabel(leftMidPlusLabel, "+");
    configureLabel(rightMidMinusLabel, "-");
    configureLabel(rightMidPlusLabel, "+");
    leftMidMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    leftMidPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightMidMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightMidPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);

    // Low +/- Labels
    configureLabel(leftLowMinusLabel, "-");
    configureLabel(leftLowPlusLabel, "+");
    configureLabel(rightLowMinusLabel, "-");
    configureLabel(rightLowPlusLabel, "+");
    leftLowMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    leftLowPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightLowMinusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);
    rightLowPlusLabel.setFont(MixerConstants::KNOB_SIGN_FONT_SIZE);

    // Add components
    // Trim
    addAndMakeVisible(leftTrimSlider);
    addAndMakeVisible(leftTrimLabel);
    addAndMakeVisible(leftTrimMinusLabel);
    addAndMakeVisible(leftTrimPlusLabel);
    addAndMakeVisible(rightTrimSlider);
    addAndMakeVisible(rightTrimLabel);
    addAndMakeVisible(rightTrimMinusLabel);
    addAndMakeVisible(rightTrimPlusLabel);

    // High
    addAndMakeVisible(leftHighSlider);
    addAndMakeVisible(leftHighLabel);
    addAndMakeVisible(leftHighPlusLabel);
    addAndMakeVisible(leftHighMinusLabel);
    addAndMakeVisible(rightHighSlider);
    addAndMakeVisible(rightHighLabel);
    addAndMakeVisible(rightHighMinusLabel);
    addAndMakeVisible(rightHighPlusLabel);

    // Mid
    addAndMakeVisible(leftMidSlider);
    addAndMakeVisible(leftMidLabel);
    addAndMakeVisible(leftMidMinusLabel);
    addAndMakeVisible(leftMidPlusLabel);
    addAndMakeVisible(rightMidSlider);
    addAndMakeVisible(rightMidLabel);
    addAndMakeVisible(rightMidMinusLabel);
    addAndMakeVisible(rightMidPlusLabel);

    // Low
    addAndMakeVisible(leftLowSlider);
    addAndMakeVisible(leftLowLabel);
    addAndMakeVisible(leftLowMinusLabel);
    addAndMakeVisible(leftLowPlusLabel);
    addAndMakeVisible(rightLowSlider);
    addAndMakeVisible(rightLowLabel);
    addAndMakeVisible(rightLowMinusLabel);
    addAndMakeVisible(rightLowPlusLabel);

    // Volume
    addAndMakeVisible(leftVolumeSlider);
    addAndMakeVisible(leftVolumeLabel);
    addAndMakeVisible(rightVolumeSlider);
    addAndMakeVisible(rightVolumeLabel);

    // Crossfader
    addAndMakeVisible(crossfaderSlider);
    addAndMakeVisible(crossfaderLabel);
}

// Destructor
MixerControls::~MixerControls() {
    // Cleanup sliders
    leftTrimSlider.setLookAndFeel(nullptr);
    rightTrimSlider.setLookAndFeel(nullptr);
    leftHighSlider.setLookAndFeel(nullptr);
    rightHighSlider.setLookAndFeel(nullptr);
    leftMidSlider.setLookAndFeel(nullptr);
    rightMidSlider.setLookAndFeel(nullptr);
    leftLowSlider.setLookAndFeel(nullptr);
    rightLowSlider.setLookAndFeel(nullptr);
    leftVolumeSlider.setLookAndFeel(nullptr);
    rightVolumeSlider.setLookAndFeel(nullptr);
    crossfaderSlider.setLookAndFeel(nullptr);
}

//===========================================================================//

//--- Programmatic Control Setters ---//
// These functions allow external code to update the mixer controls programmatically.
void MixerControls::setLeftTrimValue(float value, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    leftTrimSlider.setValue(value, notification);
    if (!notify) {
        leftTrimValue = value;
        if (onLeftTrimChanged)
            onLeftTrimChanged(leftTrimValue);
    }
}
//------------------------------//


//--- Programmatic Control Setters ---//
// These functions allow external code to update the mixer controls programmatically.
// This automatically updates Trim, EQ, Volume, and Crossfader sliders, and can optionally trigger callbacks.

void MixerControls::setRightTrimValue(float value, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    rightTrimSlider.setValue(value, notification);
    if (!notify) {
        rightTrimValue = value;
        if (onRightTrimChanged)
            onRightTrimChanged(rightTrimValue);
    }
}

void MixerControls::setLeftEQValues(float high, float mid, float low, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    leftHighSlider.setValue(high, notification);
    leftMidSlider.setValue(mid, notification);
    leftLowSlider.setValue(low, notification);

    if (!notify) {
        leftHighEQ = high;
        leftMidEQ = mid;
        leftLowEQ = low;
        if (onLeftEQChanged)
            onLeftEQChanged(leftHighEQ, leftMidEQ, leftLowEQ);
    }
}

void MixerControls::setRightEQValues(float high, float mid, float low, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    rightHighSlider.setValue(high, notification);
    rightMidSlider.setValue(mid, notification);
    rightLowSlider.setValue(low, notification);

    if (!notify) {
        rightHighEQ = high;
        rightMidEQ = mid;
        rightLowEQ = low;
        if (onRightEQChanged)
            onRightEQChanged(rightHighEQ, rightMidEQ, rightLowEQ);
    }
}

void MixerControls::setLeftVolumeValue(float value, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    leftVolumeSlider.setValue(value, notification);
    if (!notify) {
        leftVolumeGain = value;
        if (onLeftVolumeChanged)
            onLeftVolumeChanged(leftVolumeGain);
    }
}

void MixerControls::setRightVolumeValue(float value, bool notify) {
    auto notification = notify ? juce::sendNotificationSync : juce::dontSendNotification;
    rightVolumeSlider.setValue(value, notification);
    if (!notify) {
        rightVolumeGain = value;
        if (onRightVolumeChanged)
            onRightVolumeChanged(rightVolumeGain);
    }
}

//===========================================================================//


void MixerControls::paint(juce::Graphics& g) {
    auto area = getLocalBounds().toFloat();

    // Draw mixer background
    g.setColour(MixerConstants::BACKGROUND_COLOUR);
    g.fillRoundedRectangle(area, MixerConstants::CORNER_RADIUS);

    // Draw mixer border
    g.setColour(MixerConstants::STROKE_COLOUR);
    g.drawRoundedRectangle(area, MixerConstants::CORNER_RADIUS, MixerConstants::STROKE_WIDTH);
}

//===========================================================================//

void MixerControls::resized() {
    const auto area = getLocalBounds();

    using namespace MixerConstants;
    
    // Reusable calculation values
    const float arcRadius = (KNOB_OUTER_DIAMETER * 0.5f) + 2.0f;
    const int knobCenterInBounds = KNOB_BOUNDS_SIZE / 2;
    const int arcTopOffset = static_cast<int>(knobCenterInBounds - arcRadius);
    const float knobVisualRadius = KNOB_OUTER_DIAMETER * 0.5f;
    
    // Horizontal positions (same for all knobs)
    const int leftX = KNOB_MARGIN_SIDE;
    const int rightX = area.getWidth() - KNOB_MARGIN_SIDE - KNOB_BOUNDS_SIZE;
    const int leftKnobCenterX = leftX + knobCenterInBounds;
    const int rightKnobCenterX = rightX + knobCenterInBounds;
    
    // Vertical spacing between knobs
    const int knobSpacing = 4;
    const int totalKnobHeight = KNOB_BOUNDS_SIZE;
    
    // Lambda to position a knob with all its elements
    auto positionKnob = [&](juce::Slider& leftSlider, juce::Slider& rightSlider,
                            juce::Label& leftLabel, juce::Label& rightLabel,
                            juce::Label& leftMinus, juce::Label& leftPlus,
                            juce::Label& rightMinus, juce::Label& rightPlus,
                            int baseY) {
        const int knobY = baseY;
        const int arcTopY = knobY + arcTopOffset;
        const int labelY = arcTopY - KNOB_LABEL_TO_ARC_SPACING - KNOB_LABEL_HEIGHT;
        const int signY = knobY + (KNOB_BOUNDS_SIZE / 2) - (KNOB_SIGN_HEIGHT / 2);
        
        // Position labels
        leftLabel.setBounds(leftX, labelY, KNOB_BOUNDS_SIZE, KNOB_LABEL_HEIGHT);
        rightLabel.setBounds(rightX, labelY, KNOB_BOUNDS_SIZE, KNOB_LABEL_HEIGHT);
        
        // Position sliders
        leftSlider.setBounds(leftX, knobY, KNOB_BOUNDS_SIZE, KNOB_BOUNDS_SIZE);
        rightSlider.setBounds(rightX, knobY, KNOB_BOUNDS_SIZE, KNOB_BOUNDS_SIZE);
        
        // Position +/- signs
        leftMinus.setBounds(static_cast<int>(leftKnobCenterX - knobVisualRadius - KNOB_SIGN_SPACING - KNOB_SIGN_WIDTH),
                        signY, KNOB_SIGN_WIDTH, KNOB_SIGN_HEIGHT);
        leftPlus.setBounds(static_cast<int>(leftKnobCenterX + knobVisualRadius + KNOB_SIGN_SPACING),
                        signY, KNOB_SIGN_WIDTH, KNOB_SIGN_HEIGHT);
        rightMinus.setBounds(static_cast<int>(rightKnobCenterX - knobVisualRadius - KNOB_SIGN_SPACING - KNOB_SIGN_WIDTH),
                            signY, KNOB_SIGN_WIDTH, KNOB_SIGN_HEIGHT);
        rightPlus.setBounds(static_cast<int>(rightKnobCenterX + knobVisualRadius + KNOB_SIGN_SPACING),
                        signY, KNOB_SIGN_WIDTH, KNOB_SIGN_HEIGHT);
    };
    
    // Position all knobs with 4px spacing between them
    int currentY = KNOB_MARGIN_TOP;
    
    positionKnob(leftTrimSlider, rightTrimSlider, leftTrimLabel, rightTrimLabel,
                leftTrimMinusLabel, leftTrimPlusLabel, rightTrimMinusLabel, rightTrimPlusLabel,
                currentY);
    
    currentY += totalKnobHeight + knobSpacing;
    positionKnob(leftHighSlider, rightHighSlider, leftHighLabel, rightHighLabel,
                leftHighMinusLabel, leftHighPlusLabel, rightHighMinusLabel, rightHighPlusLabel,
                currentY);
    
    currentY += totalKnobHeight + knobSpacing;
    positionKnob(leftMidSlider, rightMidSlider, leftMidLabel, rightMidLabel,
                leftMidMinusLabel, leftMidPlusLabel, rightMidMinusLabel, rightMidPlusLabel,
                currentY);
    
    currentY += totalKnobHeight + knobSpacing;
    const int lowBaseY = currentY; // store baseY for low knobs
    positionKnob(leftLowSlider, rightLowSlider, leftLowLabel, rightLowLabel,
                leftLowMinusLabel, leftLowPlusLabel, rightLowMinusLabel, rightLowPlusLabel,
                lowBaseY);

    // Volume sliders positioned 4px below the visual bottom of the Low knobs
    const int leftVolumeX = leftKnobCenterX - (VOLUME_SLIDER_WIDTH / 2);
    const int rightVolumeX = rightKnobCenterX - (VOLUME_SLIDER_WIDTH / 2);

    const int lowKnobVisualBottom = lowBaseY + static_cast<int>((KNOB_BOUNDS_SIZE - KNOB_OUTER_DIAMETER) / 2 + KNOB_OUTER_DIAMETER);
    const int volumeY = lowKnobVisualBottom + 4; // 4px gap
    const int volumeLabelY = volumeY + VOLUME_SLIDER_HEIGHT + VOLUME_LABEL_TOP_GAP; // 4px gap to label

    const int volumeLabelWidth = 60;
    const int leftVolumeCenterX = leftVolumeX + (VOLUME_SLIDER_WIDTH / 2);
    const int rightVolumeCenterX = rightVolumeX + (VOLUME_SLIDER_WIDTH / 2);

    leftVolumeSlider.setBounds(leftVolumeX, volumeY, VOLUME_SLIDER_WIDTH, VOLUME_SLIDER_HEIGHT);
    rightVolumeSlider.setBounds(rightVolumeX, volumeY, VOLUME_SLIDER_WIDTH, VOLUME_SLIDER_HEIGHT);

    leftVolumeLabel.setBounds(leftVolumeCenterX - (volumeLabelWidth / 2), volumeLabelY,
                                volumeLabelWidth, KNOB_LABEL_HEIGHT);
    rightVolumeLabel.setBounds(rightVolumeCenterX - (volumeLabelWidth / 2), volumeLabelY,
                                volumeLabelWidth, KNOB_LABEL_HEIGHT);

    // Crossfader positioned below volume sliders, centered horizontally
    const int crossfaderWidth = area.getWidth() - (2 * KNOB_MARGIN_SIDE); // mixer width - 32
    const int crossfaderX = KNOB_MARGIN_SIDE; // equal left/right margins
    const int crossfaderY = volumeLabelY + KNOB_LABEL_HEIGHT ; // move up by 12px
    const int crossfaderLabelY = crossfaderY + CROSSFADER_SLIDER_HEIGHT + CROSSFADER_LABEL_TOP_GAP;

    crossfaderSlider.setBounds(crossfaderX, crossfaderY, crossfaderWidth, CROSSFADER_SLIDER_HEIGHT);
    
    const int crossfaderLabelWidth = 80;
    const int crossfaderCenterX = crossfaderX + (crossfaderWidth / 2);
    crossfaderLabel.setBounds(crossfaderCenterX - (crossfaderLabelWidth / 2), crossfaderLabelY,
                            crossfaderLabelWidth, KNOB_LABEL_HEIGHT);
}