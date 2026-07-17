/*
    FaderLookAndFeel.h
    Custom LookAndFeel for faders and rotary knobs.
    The main responsibility of this class is to override the default JUCE LookAndFeel methods to provide a 
    unique visual style for the faders and rotary knobs in the DJ App application.
    It defines specific colors, dimensions, and drawing logic to create a cohesive and visually appealing user interface 
    that matches the overall design aesthetic of the application.
*/


#pragma once
#include <JuceHeader.h>


//--- DJ App LookAndFeel definition ---//
class DJAppLookAndFeel : public juce::LookAndFeel_V4 {
    private:
        // Fader constants
        // Used for both vertical and horizontal faders.
        static constexpr float FADER_TRACK_THICKNESS = 4.0f;
        static constexpr float FADER_THUMB_RADIUS = 2.0f;
        static constexpr float FADER_THUMB_WIDTH_VERTICAL = 30.0f;
        static constexpr float FADER_THUMB_HEIGHT_VERTICAL = 10.0f;
        static constexpr float FADER_THUMB_WIDTH_HORIZONTAL = 10.0f;
        static constexpr float FADER_THUMB_HEIGHT_HORIZONTAL = 30.0f;
        static constexpr float FADER_LINE_WIDTH = 14.0f;
        static constexpr float FADER_LINE_HEIGHT = 1.0f;

        const juce::Colour faderInactiveColour {29, 31, 32};
        const juce::Colour faderActiveColour {66, 189, 17};
        const juce::Colour faderThumbColour {89, 89, 96};
        const juce::Colour faderThumbLineColour {38, 38, 44};

        // Knob constants
        static constexpr float KNOB_OUTER_DIAMETER = 32.0f;
        static constexpr float KNOB_INNER_DIAMETER = 24.0f;
        static constexpr float KNOB_INDICATOR_STROKE = 0.5f;
        static constexpr float KNOB_PROGRESS_STROKE = 3.0f;

        const juce::Colour knobOuterColour {28, 28, 32};
        const juce::Colour knobInnerColour {33, 33, 38};
        const juce::Colour knobIndicatorColour {215, 215, 215};
        const juce::Colour knobProgressColour {100, 200, 100};
        const juce::Colour knobTrackColour {39, 39, 43};

        // Fader helpers
        void drawFaderTrack(juce::Graphics& g, const juce::Rectangle<float>& track, float activeLength, bool isVertical) const
        {
            // Draw inactive track
            g.setColour(faderInactiveColour);
            g.fillRoundedRectangle(track, FADER_TRACK_THICKNESS * 0.5f);

            // Draw active portion
            if (activeLength > 0.0f) {
                juce::Rectangle<float> active = isVertical
                    ? juce::Rectangle<float>(track.getX(), track.getBottom() - activeLength, FADER_TRACK_THICKNESS, activeLength)
                    : juce::Rectangle<float>(track.getX(), track.getY(), activeLength, FADER_TRACK_THICKNESS);

                // Fill active portion
                g.setColour(faderActiveColour);
                g.fillRoundedRectangle(active, FADER_TRACK_THICKNESS * 0.5f);
            }
        }

    // Knob helpers
        void drawKnobArc(juce::Graphics& g, const juce::Point<float>& centre, float arcRadius,
                        float startAngle, float endAngle, const juce::Colour& colour) const
        {
            // Progress arc around knob
            juce::Path arc;
            arc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
            g.setColour(colour);
            g.strokePath(arc, juce::PathStrokeType(KNOB_PROGRESS_STROKE));
        }

        // Indicator line
        void drawKnobIndicator(juce::Graphics& g, const juce::Point<float>& centre, float innerRadius, float angle) const
        {
            juce::Path indicator;
            indicator.startNewSubPath(centre.x, centre.y - innerRadius);
            indicator.lineTo(centre.x, centre.y);
            indicator.applyTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));

            g.setColour(knobIndicatorColour);
            g.strokePath(indicator, juce::PathStrokeType(KNOB_INDICATOR_STROKE));
        }

    public:
        // Purpose: Draw a custom linear slider (fader/crossfader).
        // Inputs: graphics context, bounds, slider position values, slider style, slider reference.
        // Outputs: none.
        void drawLinearSlider(juce::Graphics& g,
                            int x, int y, int width, int height,
                            float sliderPos,
                            float minSliderPos, float maxSliderPos,
                            const juce::Slider::SliderStyle style,
                            juce::Slider& slider) override
        {
            juce::ignoreUnused(sliderPos, minSliderPos, maxSliderPos, style);

            const bool isVertical = slider.isVertical();
            const float proportion = juce::jlimit(0.0f, 1.0f, static_cast<float>(slider.valueToProportionOfLength(slider.getValue())));

            // Vertical fader rendering
            if (isVertical) {
                const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
                juce::Rectangle<float> track(centreX - FADER_TRACK_THICKNESS * 0.5f, static_cast<float>(y), FADER_TRACK_THICKNESS, static_cast<float>(height));
                const float thumbY = track.getBottom() - proportion * track.getHeight();

                drawFaderTrack(g, track, track.getBottom() - thumbY, true);

                juce::Rectangle<float> thumb(FADER_THUMB_WIDTH_VERTICAL, FADER_THUMB_HEIGHT_VERTICAL);
                thumb.setCentre(centreX, thumbY);
                g.setColour(faderThumbColour);
                g.fillRoundedRectangle(thumb, FADER_THUMB_RADIUS);

                juce::Rectangle<float> line(FADER_LINE_WIDTH, FADER_LINE_HEIGHT);
                line.setCentre(thumb.getCentre());
                g.setColour(faderThumbLineColour);
                g.fillRect(line);
            } 

            // Horizontal fader rendering
            else {
                const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
                juce::Rectangle<float> track(static_cast<float>(x), centreY - FADER_TRACK_THICKNESS * 0.5f, static_cast<float>(width), FADER_TRACK_THICKNESS);
                const float thumbX = track.getX() + proportion * track.getWidth();
                const float centerTrackX = track.getX() + track.getWidth() * 0.5f;

                // Draw inactive track
                g.setColour(faderInactiveColour);
                g.fillRoundedRectangle(track, FADER_TRACK_THICKNESS * 0.5f);

                // Draw active portion from center to playhead (left or right)
                if (std::abs(proportion - 0.5f) > 0.02f) {
                    juce::Rectangle<float> active;
                    if (thumbX < centerTrackX) {
                        // Moving left: active from thumbX to center
                        active = juce::Rectangle<float>(thumbX, track.getY(), centerTrackX - thumbX, FADER_TRACK_THICKNESS);
                    } else {
                        // Moving right: active from center to thumbX
                        active = juce::Rectangle<float>(centerTrackX, track.getY(), thumbX - centerTrackX, FADER_TRACK_THICKNESS);
                    }
                    g.setColour(faderActiveColour);
                    g.fillRoundedRectangle(active, FADER_TRACK_THICKNESS * 0.5f);
                }

                // Draw thumb
                juce::Rectangle<float> thumb(FADER_THUMB_WIDTH_HORIZONTAL, FADER_THUMB_HEIGHT_HORIZONTAL);
                thumb.setCentre(thumbX, centreY);
                g.setColour(faderThumbColour);
                g.fillRoundedRectangle(thumb, FADER_THUMB_RADIUS);

                // Draw thumb line
                juce::Rectangle<float> line(FADER_LINE_HEIGHT, FADER_LINE_WIDTH);
                line.setCentre(thumb.getCentre());
                g.setColour(faderThumbLineColour);
                g.fillRect(line);
            }
        }

        // Purpose: Draw a custom rotary slider (knob).
        // Inputs: graphics context, bounds, slider position, angle range, slider reference.
        // Outputs: none.
        void drawRotarySlider(juce::Graphics& g,
                            int x, int y, int width, int height,
                            float sliderPos,
                            float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            const auto centre = bounds.getCentre();

            const float outerRadius = KNOB_OUTER_DIAMETER * 0.5f;
            const float innerRadius = KNOB_INNER_DIAMETER * 0.5f;
            const float arcRadius = outerRadius + 2.0f;

            // Background track
            drawKnobArc(g, centre, arcRadius, rotaryStartAngle, rotaryEndAngle, knobTrackColour);

            // Outer and inner circles
            g.setColour(knobOuterColour);
            g.fillEllipse(centre.x - outerRadius, centre.y - outerRadius, KNOB_OUTER_DIAMETER, KNOB_OUTER_DIAMETER);

            g.setColour(knobInnerColour);
            g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius, KNOB_INNER_DIAMETER, KNOB_INNER_DIAMETER);

            // Current angle and progress arc
            const float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
            const float centreAngle = rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle);

            if (std::abs(currentAngle - centreAngle) > 0.02f) {
                const float startAngle = currentAngle > centreAngle ? centreAngle : currentAngle;
                const float endAngle = currentAngle > centreAngle ? currentAngle : centreAngle;
                drawKnobArc(g, centre, arcRadius, startAngle, endAngle, knobProgressColour);
            }

            // Indicator line
            drawKnobIndicator(g, centre, innerRadius, currentAngle);
        }
};