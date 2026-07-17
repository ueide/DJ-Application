/*
    AudioAnalyzer.cpp
    Implements BPM detection and beat grid generation.
    Main responsibilities:
    - Analyze audio files to extract BPM and beat positions.
*/

#include "AudioAnalyzer.h"


AudioAnalyzer::AudioAnalyzer() {}

AudioAnalyzer::~AudioAnalyzer() {}


//--- Audio file analysis ---//
AudioAnalyzer::AnalysisResult AudioAnalyzer::analyzeFile(const juce::File& file, 
                                            juce::AudioFormatManager& formatManager) {
    AnalysisResult result;
    
    // Create audio reader for the file
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) {
        return result;
    }

    // Read audio into buffer - Analyze entire file for BPM detection
    const int samplesToRead = (int)reader->lengthInSamples;
    juce::AudioBuffer<float> buffer(reader->numChannels, samplesToRead);
    reader->read(&buffer, 0, samplesToRead, 0, true, true);

    // Detect BPM
    result.bpm = detectBPM(buffer, reader->sampleRate);
    result.bpmDetected = (result.bpm > 0.0);
    
    // Generate beat grid if BPM was detected
    if (result.bpmDetected) {
        result.beatPositions = generateBeatGrid(buffer, reader->sampleRate, result.bpm);
    }

    delete reader;
    return result;
}


//--- BPM detection ---//
// R5 summary (see BPM_report.txt for full text):
// - BPM is used for beatmatching and sync between decks, plus timing loops and FX.
// - Alternatives include manual tap tempo, beat grid editing, and phase meters.
// - Concept: compute energy envelope, detect onsets, then estimate tempo from onset intervals.
double AudioAnalyzer::detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    if (buffer.getNumSamples() == 0) {
        return 0.0;
    }

    // Calculate energy envelope
    std::vector<float> energy;
    calculateEnergy(buffer, energy);

    // Detect onsets from energy envelope
    std::vector<int> onsets = detectOnsets(energy, sampleRate);

    // Estimate BPM from onset intervals
    double bpm = estimateBPMFromOnsets(onsets, sampleRate);

    return bpm;
}


//--- Calculate energy of audio buffer ---//
// This computes the short-time energy envelope of the audio signal
void AudioAnalyzer::calculateEnergy(const juce::AudioBuffer<float>& buffer, 
                                    std::vector<float>& energy) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const int numFrames = numSamples / hopSize;

    energy.resize(numFrames, 0.0f); // Initialize energy vector

    // Process each frame of audio
    for (int frame = 0; frame < numFrames; ++frame) {
        float frameEnergy = 0.0f;
        int startSample = frame * hopSize;
        int endSample = juce::jmin(startSample + hopSize, numSamples);

        // Sum energy across all channels
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = startSample; i < endSample; ++i) {
                frameEnergy += channelData[i] * channelData[i];
            }
        }

        energy[frame] = std::sqrt(frameEnergy / (endSample - startSample));
    }
}


//--- Onset detection ---//
// This works by finding local maxima in the energy envelope that exceed a dynamic threshold
std::vector<int> AudioAnalyzer::detectOnsets(const std::vector<float>& energy, double sampleRate) {
    std::vector<int> onsets;
    
    if (energy.size() < 3) {
        return onsets;
    }

    // Calculate mean energy
    float meanEnergy = 0.0f;
    for (float e : energy) {
        meanEnergy += e;
    }
    meanEnergy /= energy.size();

    // Detect peaks above threshold
    for (size_t i = 1; i < energy.size() - 1; ++i) {
        float threshold = meanEnergy * onsetThreshold;
        
        // Check if this is a local maximum above threshold
        if (energy[i] > threshold && 
            energy[i] > energy[i - 1] && 
            energy[i] > energy[i + 1]) {
            onsets.push_back(i);
        }
    }

    return onsets;
}


//--- Estimate BPM from onset intervals ---//
double AudioAnalyzer::estimateBPMFromOnsets(const std::vector<int>& onsets, double sampleRate) {
    if (onsets.size() < 2) {
        return 0.0;
    }

    // Calculate intervals between onsets
    std::vector<double> intervals;
    for (size_t i = 1; i < onsets.size(); ++i) {
        double intervalInSeconds = (onsets[i] - onsets[i - 1]) * hopSize / sampleRate;
        intervals.push_back(intervalInSeconds);
    }

    // Find median interval
    std::sort(intervals.begin(), intervals.end());
    double medianInterval = intervals[intervals.size() / 2];

    // Convert interval to BPM
    double bpm = 60.0 / medianInterval; // BPM calculation

    // Handle half-time and double-time
    while (bpm < minBPM) {
        bpm *= 2.0;
    }
    while (bpm > maxBPM) {
        bpm /= 2.0;
    }

    // Return 0 if outside valid range
    if (bpm < minBPM || bpm > maxBPM) {
        return 0.0;
    }

    return std::round(bpm);
}


//--- Generate beat grid from BPM ---//
std::vector<double> AudioAnalyzer::generateBeatGrid(const juce::AudioBuffer<float>& buffer,
                                                double sampleRate, double bpm) {
    std::vector<double> beatPositions;
    
    if (bpm <= 0.0 || buffer.getNumSamples() == 0) {
        return beatPositions;
    }
    
    // Calculate beat interval in seconds
    const double beatInterval = 60.0 / bpm;
    const double totalDuration = buffer.getNumSamples() / sampleRate;
    
    // Detect first beat onset
    std::vector<float> energy;
    calculateEnergy(buffer, energy);
    std::vector<int> onsets = detectOnsets(energy, sampleRate);
    
    // Find the first strong onset as the starting point
    double firstBeatTime = 0.0;
    if (!onsets.empty()) {
        firstBeatTime = (onsets[0] * hopSize) / sampleRate;
    }
    
    // Generate beat grid from first beat
    for (double beatTime = firstBeatTime; beatTime < totalDuration; beatTime += beatInterval) {
        beatPositions.push_back(beatTime);
    }
    
    // Add beats before the first detected beat 
    for (double beatTime = firstBeatTime - beatInterval; beatTime >= 0.0; beatTime -= beatInterval) {
        beatPositions.insert(beatPositions.begin(), beatTime);
    }
    
    return beatPositions;
}