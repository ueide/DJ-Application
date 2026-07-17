/*
    MixerControls.h
    Mixer UI with trim, 3-band EQ, volume faders, and crossfader controls.
*/


#pragma once
#include <JuceHeader.h>
#include "FaderLookAndFeel.h"


//--- MixerControls component for EQ, trim, volume, and crossfader ---//
class MixerControls : public juce::Component {
    public:
        // Purpose: Create the mixer controls component.
        // Inputs: none.
        // Outputs: initialized MixerControls instance.
        MixerControls();
        // Purpose: Release mixer control resources.
        // Inputs: none.
        // Outputs: none.
        ~MixerControls() override;

        // Purpose: Render mixer UI.
        // Inputs: graphics context.
        // Outputs: none.
        void paint(juce::Graphics&) override;
        // Purpose: Layout mixer controls.
        // Inputs: none.
        // Outputs: none.
        void resized() override;

        // Purpose: Get current left volume value.
        // Inputs: none.
        // Outputs: left volume gain.
        float getLeftVolume() const { return leftVolumeGain; }
        // Purpose: Get current right volume value.
        // Inputs: none.
        // Outputs: right volume gain.
        float getRightVolume() const { return rightVolumeGain; }
        // Purpose: Get current crossfader position.
        // Inputs: none.
        // Outputs: crossfader position.
        float getCrossfaderPosition() const { return crossfaderPosition; }
        // Purpose: Get current left trim value.
        // Inputs: none.
        // Outputs: left trim value.
        float getLeftTrimValue() const { return leftTrimValue; }
        // Purpose: Get current right trim value.
        // Inputs: none.
        // Outputs: right trim value.
        float getRightTrimValue() const { return rightTrimValue; }

        // Purpose: Set left trim value.
        // Inputs: value, notify to trigger callbacks.
        // Outputs: none.
        void setLeftTrimValue(float value, bool notify = true);
        // Purpose: Set right trim value.
        // Inputs: value, notify to trigger callbacks.
        // Outputs: none.
        void setRightTrimValue(float value, bool notify = true);
        // Purpose: Set left EQ values.
        // Inputs: high, mid, low values, notify to trigger callbacks.
        // Outputs: none.
        void setLeftEQValues(float high, float mid, float low, bool notify = true);
        // Purpose: Set right EQ values.
        // Inputs: high, mid, low values, notify to trigger callbacks.
        // Outputs: none.
        void setRightEQValues(float high, float mid, float low, bool notify = true);
        // Purpose: Set left volume value.
        // Inputs: value, notify to trigger callbacks.
        // Outputs: none.
        void setLeftVolumeValue(float value, bool notify = true);
        // Purpose: Set right volume value.
        // Inputs: value, notify to trigger callbacks.
        // Outputs: none.
        void setRightVolumeValue(float value, bool notify = true);

        // Callbacks invoked on user/programmatic changes
        std::function<void(float)> onLeftVolumeChanged;
        std::function<void(float)> onRightVolumeChanged;
        std::function<void(float)> onCrossfaderChanged;
        std::function<void(float)> onLeftTrimChanged;
        std::function<void(float)> onRightTrimChanged;
        
        // (high, mid, low)
        std::function<void(float, float, float)> onLeftEQChanged;
        std::function<void(float, float, float)> onRightEQChanged;

        // Purpose: Compute deck width for layout.
        // Inputs: totalWidth, mixerWidth.
        // Outputs: deck width.
        static int calculateDeckWidth(int totalWidth, int mixerWidth) {
            return (totalWidth / 2) - (mixerWidth / 2);
        }
        
        // Purpose: Return the left deck X position.
        // Inputs: none.
        // Outputs: x position in pixels.
        static int calculateLeftDeckX() {
            return 0;
        }
        
        // Purpose: Compute right deck X position.
        // Inputs: totalWidth, mixerWidth.
        // Outputs: x position in pixels.
        static int calculateRightDeckX(int totalWidth, int mixerWidth) {
            return (totalWidth / 2) + (mixerWidth / 2);
        }

    private:
        DJAppLookAndFeel lookAndFeel;

        // Trim controls
        juce::Slider leftTrimSlider;
        juce::Slider rightTrimSlider;

        // Trim Labels
        juce::Label leftTrimLabel;
        juce::Label rightTrimLabel;

        // Trim +/- Labels
        juce::Label leftTrimMinusLabel;
        juce::Label leftTrimPlusLabel;
        juce::Label rightTrimMinusLabel;
        juce::Label rightTrimPlusLabel;

        // High EQ controls
        juce::Slider leftHighSlider;
        juce::Slider rightHighSlider;
        juce::Label leftHighLabel;
        juce::Label rightHighLabel;
        juce::Label leftHighMinusLabel;
        juce::Label leftHighPlusLabel;
        juce::Label rightHighMinusLabel;
        juce::Label rightHighPlusLabel;

        // Mid EQ controls
        juce::Slider leftMidSlider;
        juce::Slider rightMidSlider;
        juce::Label leftMidLabel;
        juce::Label rightMidLabel;
        juce::Label leftMidMinusLabel;
        juce::Label leftMidPlusLabel;
        juce::Label rightMidMinusLabel;
        juce::Label rightMidPlusLabel;

        // Low EQ controls
        juce::Slider leftLowSlider;
        juce::Slider rightLowSlider;
        juce::Label leftLowLabel;
        juce::Label rightLowLabel;
        juce::Label leftLowMinusLabel;
        juce::Label leftLowPlusLabel;
        juce::Label rightLowMinusLabel;
        juce::Label rightLowPlusLabel;

        // Volume controls
        juce::Slider leftVolumeSlider;
        juce::Slider rightVolumeSlider;
        juce::Label leftVolumeLabel;
        juce::Label rightVolumeLabel;

        // Crossfader controls
        juce::Slider crossfaderSlider;
        juce::Label crossfaderLabel;

        // Cached control values
        float leftVolumeGain {0.5f};
        float rightVolumeGain {0.5f};
        float crossfaderPosition {0.5f};
        float leftTrimValue {0.0f};
        float rightTrimValue {0.0f};
        
        // EQ values
        float leftHighEQ {0.0f};
        float leftMidEQ {0.0f};
        float leftLowEQ {0.0f};
        float rightHighEQ {0.0f};
        float rightMidEQ {0.0f};
        float rightLowEQ {0.0f};
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerControls)
};