/*
    AudioAnalyzer.h
    BPM analysis helper for detecting tempo and generating beat grids.
*/


#pragma once
#include <JuceHeader.h>


//--- AudioAnalyzer class definition ---//
class AudioAnalyzer {
    public:
        // Analysis result structure.
        struct AnalysisResult {
            double bpm {0.0};
            bool bpmDetected {false};
            
            // Beat grid positions in seconds.
            std::vector<double> beatPositions;
        };

        // Purpose: Create an analyzer with default parameters.
        // Inputs: none.
        // Outputs: an initialized AudioAnalyzer instance.
        AudioAnalyzer();

        // Purpose: Release analyzer resources.
        // Inputs: none.
        // Outputs: none.
        ~AudioAnalyzer();

        // Purpose: Analyze an audio file and return BPM and beat grid results.
        // Inputs: file to analyze, formatManager for reader creation.
        // Outputs: AnalysisResult with BPM and beat positions.
        AnalysisResult analyzeFile(const juce::File& file, juce::AudioFormatManager& formatManager);

        // Purpose: Detect BPM from an audio buffer.
        // Inputs: audio buffer, sample rate in Hz.
        // Outputs: estimated BPM value (0 if not detected).
        double detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate);
        
        // Purpose: Generate beat positions for a buffer using a BPM.
        // Inputs: audio buffer, sample rate in Hz, BPM value.
        // Outputs: vector of beat positions in seconds.
        std::vector<double> generateBeatGrid(const juce::AudioBuffer<float>& buffer, 
                                            double sampleRate, double bpm);
    
    
    private:
        // Analysis helpers.
        void calculateEnergy(const juce::AudioBuffer<float>& buffer, std::vector<float>& energy);
        std::vector<int> detectOnsets(const std::vector<float>& energy, double sampleRate);
        double estimateBPMFromOnsets(const std::vector<int>& onsets, double sampleRate);
        
        // Analysis parameters.
        const double minBPM = 60.0;
        const double maxBPM = 200.0;
        const int hopSize = 512;
        const float onsetThreshold = 1.5f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};