/*
    MainComponent.cpp
    Primary UI and audio coordinator for DJ App.
    This is responsible for managing the main layout, coordinating between DeckGUIs, WaveformDisplays, MixerControls, and the Playlist.
*/


#include <algorithm>
#include "MainComponent.h"
#include "AudioAnalyzer.h"


MainComponent::MainComponent() {
    setSize (1280, 780);

    // 60 Hz is a good balance between responsiveness and CPU usage.
    startTimerHz(60);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio)) {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else {
        setAudioChannels (2, 2);
    }

    formatManager.registerBasicFormats();

    //--- Initialize GUI components and callbacks ---//
    addAndMakeVisible(waveformLeft);
    addAndMakeVisible(waveformRight);
    addAndMakeVisible(mixerControls);
    addAndMakeVisible(playlistHeader);
    
    // Playlist callbacks
    playlistHeader.onLoadToLeftDeck = [this](const juce::File& file) {
        loadFileIntoDeck(file, 0);
        waveformLeft.setSource(juce::URL(file));
        waveformLeft.setBeatPositions(lastAnalysisResultLeft.beatPositions);
    };
    
    playlistHeader.onLoadToRightDeck = [this](const juce::File& file) {
        loadFileIntoDeck(file, 1);
        waveformRight.setSource(juce::URL(file));
        waveformRight.setBeatPositions(lastAnalysisResultRight.beatPositions);
    };
    
    playlistHeader.onFileDropped = [this](const juce::File& file) {
        processAndAddFileToPlaylist(file);
    };

    addAndMakeVisible(addPlaylistButton);
    addPlaylistButton.onClick = [this]() {
        openFileChooser();
    };

    addAndMakeVisible(deckLeft);
    addAndMakeVisible(deckRight);


    //============= LEFT ==============// 

    //--- Setup Deck Left ---//
    deckLeft.onFileLoaded = [this] (const juce::File& file) {
        loadFileIntoDeck(file, 0);
        waveformLeft.setSource(juce::URL(file));
        waveformLeft.setBeatPositions(lastAnalysisResultLeft.beatPositions);
    };

    // Hot Cue (Left)
    deckLeft.onHotCuesChanged = [this](const std::vector<std::pair<double, int>>& markers) {
        updateHotCuesForFile(deckLeft.getLoadedFile(), markers);
        waveformLeft.setHotCueMarkers(markers);
    };

    // Setup Waveform (Left)
    waveformLeft.onSeek = [this] (double timeSeconds) {
        transportSourceLeft.setPosition(timeSeconds);
    };

    // Sync Button (Left)
    deckLeft.onSyncPressed = [this]() {
        double targetBPM = deckRight.getCurrentBPM();
        deckLeft.updateSpeedToMatchBPM(targetBPM);
    };

    // Loop Region (Left)
    deckLeft.onLoopRegionChanged = [this](std::optional<std::pair<double, double>> region) {
        waveformLeft.setLoopRegion(region);
    };
    
    // Sampler Pad Callbacks (Left)
    deckLeft.onSampleLoaded = [this](int padIndex, const juce::File& file) {
        loadSampleToPad(0, padIndex, file);
        updateSamplerPadForDeck(0, padIndex, file);
    };

    deckLeft.onSampleTrigger = [this](int padIndex) {
        triggerSample(0, padIndex);
    };

    deckLeft.onSampleCleared = [this](int padIndex) {
        clearSamplePad(0, padIndex);
        updateSamplerPadForDeck(0, padIndex, juce::File());
    };

    deckLeft.onAllSamplesCleared = [this]() {
        clearSamplerDeck(0);
        for (int i = 0; i < numSamplerPads; ++i) {
            clearSamplePad(0, i);
        }
    };


    //============ RIGHT =============// 

    //--- Setup Deck Right ---//
    deckRight.onFileLoaded = [this] (const juce::File& file) {
        loadFileIntoDeck(file, 1);
        waveformRight.setSource(juce::URL(file));
        waveformRight.setBeatPositions(lastAnalysisResultRight.beatPositions);
    };

    // Hot cue (Right)
    deckRight.onHotCuesChanged = [this](const std::vector<std::pair<double, int>>& markers) {
        updateHotCuesForFile(deckRight.getLoadedFile(), markers);
        waveformRight.setHotCueMarkers(markers);
    };

    // Setup Waveform (Right)
    waveformRight.onSeek = [this] (double timeSeconds) {
        transportSourceRight.setPosition(timeSeconds);
    };

    // Sync Button (Right)
    deckRight.onSyncPressed = [this]() {
        double targetBPM = deckLeft.getCurrentBPM();
        deckRight.updateSpeedToMatchBPM(targetBPM);
    };

    // Loop Region (Right)
    deckRight.onLoopRegionChanged = [this](std::optional<std::pair<double, double>> region) {
        waveformRight.setLoopRegion(region);
    };
    
    // Sampler Pad Callbacks (Right)
    deckRight.onSampleLoaded = [this](int padIndex, const juce::File& file) {
        loadSampleToPad(1, padIndex, file);
        updateSamplerPadForDeck(1, padIndex, file);
    };
    deckRight.onSampleTrigger = [this](int padIndex) {
        triggerSample(1, padIndex);
    };

    deckRight.onSampleCleared = [this](int padIndex) {
        clearSamplePad(1, padIndex);
        updateSamplerPadForDeck(1, padIndex, juce::File());
    };

    deckRight.onAllSamplesCleared = [this]() {
        clearSamplerDeck(1);
        for (int i = 0; i < numSamplerPads; ++i) {
            clearSamplePad(1, i);
        }
    };


    //============= MIXER =============//
    
    //--- Setup Mixer Controls callbacks ---//
    auto trimToGain = [](float value) { return juce::Decibels::decibelsToGain(value * 12.0f); };

    //--- Trim ---//
    mixerControls.onLeftTrimChanged = [this, trimToGain](float value) {
        leftTrimGain.store(trimToGain(value));
        
        MixerState state;
        state.trim = value;
        state.high = leftHighEQ.load();
        state.mid = leftMidEQ.load();
        state.low = leftLowEQ.load();
        state.volume = leftVolumeGain.load();
        updateMixerStateForFile(deckLeft.getLoadedFile(), state);
    };
    
    mixerControls.onRightTrimChanged = [this, trimToGain](float value) {
        rightTrimGain.store(trimToGain(value));
        
        MixerState state;
        state.trim = value;
        state.high = rightHighEQ.load();
        state.mid = rightMidEQ.load();
        state.low = rightLowEQ.load();
        state.volume = rightVolumeGain.load();
        updateMixerStateForFile(deckRight.getLoadedFile(), state);
    };
    //------------------------//
    

    //--- Setup EQ callbacks ---//
    // EQ values are stored in the mixer state for each file
    mixerControls.onLeftEQChanged = [this](float high, float mid, float low) {
        leftHighEQ.store(high);
        leftMidEQ.store(mid);
        leftLowEQ.store(low);
        updateEQFilters();
        
        // Update mixer state for current file
        MixerState state;
        state.trim = mixerControls.getLeftTrimValue();
        state.high = high;
        state.mid = mid;
        state.low = low;
        state.volume = leftVolumeGain.load();
        updateMixerStateForFile(deckLeft.getLoadedFile(), state);
    };
    
    // EQ values are stored in the mixer state for each file
    mixerControls.onRightEQChanged = [this](float high, float mid, float low) {
        rightHighEQ.store(high);
        rightMidEQ.store(mid);
        rightLowEQ.store(low);
        updateEQFilters();
        
        // Update mixer state for current file
        MixerState state;
        state.trim = mixerControls.getRightTrimValue();
        state.high = high;
        state.mid = mid;
        state.low = low;
        state.volume = rightVolumeGain.load();
        updateMixerStateForFile(deckRight.getLoadedFile(), state);
    };

    //--- Volume ---//
    // Volume values are stored in the mixer state for each file
    mixerControls.onLeftVolumeChanged = [this](float gain) {
        leftVolumeGain.store(gain);
        
        // Update mixer state for current file
        MixerState state;
        state.trim = mixerControls.getLeftTrimValue();
        state.high = leftHighEQ.load();
        state.mid = leftMidEQ.load();
        state.low = leftLowEQ.load();
        state.volume = gain;
        updateMixerStateForFile(deckLeft.getLoadedFile(), state);
    };

    // Volume values are stored in the mixer state for each file
    mixerControls.onRightVolumeChanged = [this](float gain) {
        rightVolumeGain.store(gain);
        
        // Update mixer state for current file
        MixerState state;
        state.trim = mixerControls.getRightTrimValue();
        state.high = rightHighEQ.load();
        state.mid = rightMidEQ.load();
        state.low = rightLowEQ.load();
        state.volume = gain;
        updateMixerStateForFile(deckRight.getLoadedFile(), state);
    };

    //--- Crossfader ---//
    mixerControls.onCrossfaderChanged = [this](float position) {
        crossfaderPosition.store(position);
    };

    //--- Load saved playlist ---//
    playlistHeader.loadPlaylist();

    //--- Load saved hot cues ---//
    loadHotCuesStore();

    //--- Load saved mixer state ---//
    loadMixerStateStore();

    //--- Load saved sampler assignments (per deck) ---//
    loadSamplerStateStore();
    deckLeft.setSamplerPadsFromFiles(getSamplerFilesForDeck(0));
    deckRight.setSamplerPadsFromFiles(getSamplerFilesForDeck(1));
}


// Destructor
MainComponent::~MainComponent(){
    shutdownAudio();
    stopTimer();
    
    // Clean up resampling sources
    if (resamplingSourceLeft != nullptr) {
        delete resamplingSourceLeft;
        resamplingSourceLeft = nullptr;
    }
    if (resamplingSourceRight != nullptr) {
        delete resamplingSourceRight;
        resamplingSourceRight = nullptr;
    }
    
    // Clean up sample readers
    for (int i = 0; i < numSamplerPads; ++i) {
        if (sampleReaderLeft[i] != nullptr) {
            delete sampleReaderLeft[i];
            sampleReaderLeft[i] = nullptr;
        }
        if (sampleReaderRight[i] != nullptr) {
            delete sampleReaderRight[i];
            sampleReaderRight[i] = nullptr;
        }
    }
}


//--- Load audio file into specified deck ---//
void MainComponent::loadFileIntoDeck(const juce::File& file, int deckIndex) {
    auto* reader = formatManager.createReaderFor(file);
    if(reader != nullptr) {

        // Extract Metadata
        juce::String title = reader->metadataValues.getValue("title", "");
        juce::String artist = reader->metadataValues.getValue("artist", "");

        // Default values if metadata missing
        if (title.isEmpty()) {
            title = file.getFileNameWithoutExtension();
        }
        if (artist.isEmpty()) {
            artist = "Unknown Artist";
        }

        // Get BPM
        juce::String bpmString = reader->metadataValues.getValue("bpm", "");
        double bpm = 0.0;
        AudioAnalyzer::AnalysisResult analysisResult;
        bool needsAnalysis = false;
        
        // Try to get BPM from metadata first
        if (!bpmString.isEmpty()) {
            bpm = bpmString.getDoubleValue();
        }

        // Analyze for BPM if not in metadata
        if (bpm == 0.0) {
            DBG("No BPM in metadata, analyzing file: " + file.getFileName());
            analysisResult = audioAnalyzer.analyzeFile(file, formatManager);
            if (analysisResult.bpmDetected) {
                bpm = analysisResult.bpm;
                DBG("Detected BPM: " + juce::String(bpm));
                needsAnalysis = false;  // Already analyzed
            } else {
                DBG("BPM detection failed");
                needsAnalysis = true;  // Will need to analyze for waveform
            }
        } else {
            needsAnalysis = true;  // Have BPM but need analysis for waveform
        }
        
        // Analyze for waveform data if needed
        if (needsAnalysis) {
            DBG("Analyzing file for waveform visualization: " + file.getFileName());
            analysisResult = audioAnalyzer.analyzeFile(file, formatManager);
        }

        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

        // Set up transport and resampling sources based on deck index
        if(deckIndex == 0) {
            // Stop playback and clear the previous source
            transportSourceLeft.stop();
            transportSourceLeft.setSource(nullptr);
            
            // Delete old resampling source if it exists
            if (resamplingSourceLeft != nullptr) {
                delete resamplingSourceLeft;
                resamplingSourceLeft = nullptr;
            }
            
            // Set reader to transport
            readerSourceLeft = newSource.release();
            transportSourceLeft.setSource(readerSourceLeft, 0, nullptr, reader->sampleRate);
            
            // Create resampling source wrapping the transport
            resamplingSourceLeft = new juce::ResamplingAudioSource(&transportSourceLeft, false);
            resamplingSourceLeft->setResamplingRatio(1.0); // Start at normal speed
            
            // Prepare the resampling source if audio device is already running
            if (currentSampleRate > 0) {
                resamplingSourceLeft->prepareToPlay(currentBufferSize, currentSampleRate);
            }
            
            // Give deck GUI access to resampling source for tempo control
            deckLeft.setResamplingSource(resamplingSourceLeft);
            deckLeft.updateMetadata(title, artist, bpm);
            deckLeft.setLoadedFile(file);
            deckLeft.setHotCuesFromSeconds(getHotCuesForFile(file));

            const MixerState leftState = getMixerStateForFile(file);
            mixerControls.setLeftTrimValue(leftState.trim, false);
            mixerControls.setLeftEQValues(leftState.high, leftState.mid, leftState.low, false);
            mixerControls.setLeftVolumeValue(leftState.volume, false);
            updateMixerStateForFile(file, leftState);
            
            // Store analysis result for waveform
            lastAnalysisResultLeft = analysisResult;
        }
        else {
            // Stop playback and clear the previous source
            transportSourceRight.stop();
            transportSourceRight.setSource(nullptr);
            
            // Delete old resampling source if it exists
            if (resamplingSourceRight != nullptr) {
                delete resamplingSourceRight;
                resamplingSourceRight = nullptr;
            }
            
            // Set reader to transport
            readerSourceRight = newSource.release();
            transportSourceRight.setSource(readerSourceRight, 0, nullptr, reader->sampleRate);
            
            // Create resampling source wrapping the transport
            resamplingSourceRight = new juce::ResamplingAudioSource(&transportSourceRight, false);
            resamplingSourceRight->setResamplingRatio(1.0);
            
            // Prepare the resampling source if audio device is already running
            if (currentSampleRate > 0) {
                resamplingSourceRight->prepareToPlay(currentBufferSize, currentSampleRate);
            }
            
            // Give deck GUI access to resampling source for tempo control
            deckRight.setResamplingSource(resamplingSourceRight);
            deckRight.updateMetadata(title, artist, bpm);
            deckRight.setLoadedFile(file);
            deckRight.setHotCuesFromSeconds(getHotCuesForFile(file));

            const MixerState rightState = getMixerStateForFile(file);
            mixerControls.setRightTrimValue(rightState.trim, false);
            mixerControls.setRightEQValues(rightState.high, rightState.mid, rightState.low, false);
            mixerControls.setRightVolumeValue(rightState.volume, false);
            updateMixerStateForFile(file, rightState);
            
            // Store analysis result for waveform
            lastAnalysisResultRight = analysisResult;
        }
    }
}


//--- Get hot cues file path ---//
juce::File MainComponent::getHotCuesFile() const {
    auto docsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docsFolder.getChildFile("DJ App").getChildFile("hotcues.json");
}


//--- Load hot cues from file ---//
void MainComponent::loadHotCuesStore() {
    hotCueStore.clear();

    juce::File hotCuesFile = getHotCuesFile();
    if (!hotCuesFile.existsAsFile()) {
        return;
    }

    // Load and parse JSON
    juce::String jsonText = hotCuesFile.loadFileAsString();
    juce::var parsedData = juce::JSON::parse(jsonText);
    if (!parsedData.isArray()) {
        return;
    }

    for (int i = 0; i < parsedData.size(); ++i) {
        const auto& entry = parsedData[i];
        if (!entry.isObject()) {
            continue;
        }

        juce::String path = entry.getProperty("path", "").toString();
        auto cuesVar = entry.getProperty("cues", juce::var());
        if (path.isEmpty() || !cuesVar.isArray()) {
            continue;
        }

        std::vector<double> cues(hotCuePadCount, -1.0);
        for (int c = 0; c < cuesVar.size() && c < hotCuePadCount; ++c) {
            cues[(size_t)c] = (double)cuesVar[c];
        }

        hotCueStore[path] = cues;
    }
}


//--- Save hot cues to file ---//
void MainComponent::saveHotCuesStore() const {
    juce::var entries;
    entries = juce::var(juce::Array<juce::var>());

    for (const auto& item : hotCueStore) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("path", item.first);

        juce::Array<juce::var> cuesArray;
        for (double cue : item.second) {
            cuesArray.add(cue);
        }
        obj->setProperty("cues", cuesArray);

        entries.append(juce::var(obj.get()));
    }

    juce::File hotCuesFile = getHotCuesFile();
    hotCuesFile.getParentDirectory().createDirectory();

    juce::String jsonText = juce::JSON::toString(entries, true);
    hotCuesFile.replaceWithText(jsonText);
}


//--- Get hot cues for a specific file ---//
std::vector<double> MainComponent::getHotCuesForFile(const juce::File& file) const {
    std::vector<double> cues(hotCuePadCount, -1.0);
    if (file == juce::File()) {
        return cues;
    }

    auto it = hotCueStore.find(file.getFullPathName());
    if (it != hotCueStore.end()) {
        cues = it->second;
    }

    return cues;
}


//--- Update hot cues for a specific file ---//
void MainComponent::updateHotCuesForFile(const juce::File& file,
                                const std::vector<std::pair<double, int>>& markers) {

    // Markers are pairs of (time in seconds, pad index)
    if (file == juce::File() || !file.existsAsFile()) {
        return;
    }

    // Create a vector of cues initialized to -1 (indicating no cue)
    std::vector<double> cues(hotCuePadCount, -1.0);
    for (const auto& marker : markers) {
        const int padIndex = marker.second;
        if (padIndex >= 0 && padIndex < hotCuePadCount) {
            cues[(size_t)padIndex] = marker.first;
        }
    }

    const bool hasAnyCue = std::any_of(cues.begin(), cues.end(), [](double value) {
        return value >= 0.0;
    });

    // Update the store: if there are any cues, save them; if not, remove the entry for this file
    if (hasAnyCue) {
        hotCueStore[file.getFullPathName()] = cues;
    } else {
        hotCueStore.erase(file.getFullPathName());
    }

    saveHotCuesStore();
}


//--- Get sampler state file path ---//
juce::File MainComponent::getSamplerStateFile() const {
    auto docsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docsFolder.getChildFile("DJ App").getChildFile("sampler.json");
}


//--- Load sampler state from file ---//
void MainComponent::loadSamplerStateStore() {
    leftSamplerStore.fill(juce::String());
    rightSamplerStore.fill(juce::String());

    juce::File samplerFile = getSamplerStateFile();
    if (!samplerFile.existsAsFile()) {
        return;
    }

    juce::String jsonText = samplerFile.loadFileAsString();
    juce::var parsedData = juce::JSON::parse(jsonText);
    if (!parsedData.isObject()) {
        return;
    }

    auto loadDeck = [this](const juce::var& deckVar, bool isLeft) {
        if (!deckVar.isArray()) {
            return;
        }

        auto& store = isLeft ? leftSamplerStore : rightSamplerStore;
        for (int i = 0; i < numSamplerPads && i < deckVar.size(); ++i) {
            juce::String path = deckVar[i].toString();
            juce::File sampleFile(path);
            if (!path.isEmpty() && sampleFile.existsAsFile()) {
                store[(size_t)i] = path;
            } else {
                store[(size_t)i].clear();
            }
        }
    };

    loadDeck(parsedData.getProperty("left", juce::var()), true);
    loadDeck(parsedData.getProperty("right", juce::var()), false);
}


//--- Save sampler state to file ---//
void MainComponent::saveSamplerStateStore() const {
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    juce::Array<juce::var> leftArray;
    juce::Array<juce::var> rightArray;

    for (int i = 0; i < numSamplerPads; ++i) {
        leftArray.add(leftSamplerStore[(size_t)i]);
        rightArray.add(rightSamplerStore[(size_t)i]);
    }

    root->setProperty("left", leftArray);
    root->setProperty("right", rightArray);

    juce::File samplerFile = getSamplerStateFile();
    samplerFile.getParentDirectory().createDirectory();
    samplerFile.replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
}


//--- Get sampler files for a deck ---//
std::vector<juce::File> MainComponent::getSamplerFilesForDeck(int deckIndex) const {
    std::vector<juce::File> files;
    files.resize(numSamplerPads);

    const auto& store = (deckIndex == 0) ? leftSamplerStore : rightSamplerStore;
    for (int i = 0; i < numSamplerPads; ++i) {
        const juce::String& path = store[(size_t)i];
        juce::File sampleFile(path);
        if (!path.isEmpty() && sampleFile.existsAsFile()) {
            files[(size_t)i] = sampleFile;
        }
    }

    return files;
}


//--- Update one sampler pad for a deck ---//
void MainComponent::updateSamplerPadForDeck(int deckIndex, int padIndex, const juce::File& file) {
    if (padIndex < 0 || padIndex >= numSamplerPads) {
        return;
    }

    auto& store = (deckIndex == 0) ? leftSamplerStore : rightSamplerStore;
    if (file != juce::File{} && file.existsAsFile()) {
        store[(size_t)padIndex] = file.getFullPathName();
    } else {
        store[(size_t)padIndex].clear();
    }

    saveSamplerStateStore();
}


//--- Clear all sampler pads for a deck in persistence ---//
void MainComponent::clearSamplerDeck(int deckIndex) {
    auto& store = (deckIndex == 0) ? leftSamplerStore : rightSamplerStore;
    for (int i = 0; i < numSamplerPads; ++i) {
        store[(size_t)i].clear();
    }

    saveSamplerStateStore();
}


//--- Get mixer state file path ---//
juce::File MainComponent::getMixerStateFile() const {
    auto docsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docsFolder.getChildFile("DJ App").getChildFile("mixer.json");
}


//--- Load mixer state from file ---//
void MainComponent::loadMixerStateStore() {
    mixerStateStore.clear();

    juce::File mixerFile = getMixerStateFile();
    if (!mixerFile.existsAsFile()) {
        return;
    }

    juce::String jsonText = mixerFile.loadFileAsString();
    juce::var parsedData = juce::JSON::parse(jsonText);
    if (!parsedData.isArray()) {
        return;
    }

    for (int i = 0; i < parsedData.size(); ++i) {
        const auto& entry = parsedData[i];
        if (!entry.isObject()) {
            continue;
        }

        juce::String path = entry.getProperty("path", "").toString();
        if (path.isEmpty()) {
            continue;
        }

        auto parseState = [](const juce::var& node, MixerState fallback) {
            MixerState state = fallback;
            if (!node.isObject()) {
                return state;
            }

            state.trim = (float)node.getProperty("trim", state.trim);
            state.high = (float)node.getProperty("high", state.high);
            state.mid = (float)node.getProperty("mid", state.mid);
            state.low = (float)node.getProperty("low", state.low);
            state.volume = (float)node.getProperty("volume", state.volume);
            return state;
        };

        MixerStateRecord record;

        const juce::var originalVar = entry.getProperty("original", juce::var());
        const juce::var lastVar = entry.getProperty("last", juce::var());

        if (originalVar.isObject() || lastVar.isObject()) {
            record.original = parseState(originalVar, MixerState{});
            record.last = parseState(lastVar, record.original);
            record.hasOriginal = true;
        } else {
            MixerState legacyState;
            legacyState.trim = (float)entry.getProperty("trim", legacyState.trim);
            legacyState.high = (float)entry.getProperty("high", legacyState.high);
            legacyState.mid = (float)entry.getProperty("mid", legacyState.mid);
            legacyState.low = (float)entry.getProperty("low", legacyState.low);
            legacyState.volume = (float)entry.getProperty("volume", legacyState.volume);

            record.original = legacyState;
            record.last = legacyState;
            record.hasOriginal = true;
        }

        mixerStateStore[path] = record;
    }
}


//--- Save mixer state to file ---//
void MainComponent::saveMixerStateStore() const {
    juce::var entries;
    entries = juce::var(juce::Array<juce::var>());

    for (const auto& item : mixerStateStore) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("path", item.first);

        juce::DynamicObject::Ptr originalObj = new juce::DynamicObject();
        originalObj->setProperty("trim", item.second.original.trim);
        originalObj->setProperty("high", item.second.original.high);
        originalObj->setProperty("mid", item.second.original.mid);
        originalObj->setProperty("low", item.second.original.low);
        originalObj->setProperty("volume", item.second.original.volume);

        juce::DynamicObject::Ptr lastObj = new juce::DynamicObject();
        lastObj->setProperty("trim", item.second.last.trim);
        lastObj->setProperty("high", item.second.last.high);
        lastObj->setProperty("mid", item.second.last.mid);
        lastObj->setProperty("low", item.second.last.low);
        lastObj->setProperty("volume", item.second.last.volume);

        obj->setProperty("original", juce::var(originalObj.get()));
        obj->setProperty("last", juce::var(lastObj.get()));
        entries.append(juce::var(obj.get()));
    }

    juce::File mixerFile = getMixerStateFile();
    mixerFile.getParentDirectory().createDirectory();

    juce::String jsonText = juce::JSON::toString(entries, true);
    mixerFile.replaceWithText(jsonText);
}


//--- Get mixer state for a specific file ---//
MainComponent::MixerState MainComponent::getMixerStateForFile(const juce::File& file) const {
    MixerState state;
    if (file == juce::File()) {
        return state;
    }

    auto it = mixerStateStore.find(file.getFullPathName());
    if (it != mixerStateStore.end()) {
        state = it->second.last;
    }

    return state;
}


//--- Update mixer state for a specific file ---//
void MainComponent::updateMixerStateForFile(const juce::File& file, const MixerState& state) {
    // If the file is invalid, do not update the store
    if (file == juce::File() || !file.existsAsFile()) {
        return;
    }

    const juce::String path = file.getFullPathName();
    auto it = mixerStateStore.find(path);

    if (it == mixerStateStore.end()) {
        MixerStateRecord record;
        record.original = state;
        record.last = state;
        record.hasOriginal = true;
        mixerStateStore[path] = record;
    } else {
        if (!it->second.hasOriginal) {
            it->second.original = state;
            it->second.hasOriginal = true;
        }
        it->second.last = state;
    }

    saveMixerStateStore();
}


// --- Update EQ Filters --- //
void MainComponent::updateEQFilters() {
    if (currentSampleRate <= 0) return;
    
    // Get EQ values
    float lHigh = leftHighEQ.load();
    float lMid = leftMidEQ.load();
    float lLow = leftLowEQ.load();
    float rHigh = rightHighEQ.load();
    float rMid = rightMidEQ.load();
    float rLow = rightLowEQ.load();
    
    // Convert EQ values (-1 to 1) to gain in dB (-12dB to +12dB)
    auto eqToDb = [](float eqValue) { return eqValue * 12.0f; };
    
    // Left deck filters
    auto leftLowCoeffs = juce::IIRCoefficients::makeLowShelf(currentSampleRate, 250.0, 0.7, 
                                                    juce::Decibels::decibelsToGain(eqToDb(lLow)));
    auto leftMidCoeffs = juce::IIRCoefficients::makePeakFilter(currentSampleRate, 1000.0, 1.0,
                                                    juce::Decibels::decibelsToGain(eqToDb(lMid)));
    auto leftHighCoeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 4000.0, 0.7,
                                                    juce::Decibels::decibelsToGain(eqToDb(lHigh)));
    
    // Right deck filters
    auto rightLowCoeffs = juce::IIRCoefficients::makeLowShelf(currentSampleRate, 250.0, 0.7,
                                                    juce::Decibels::decibelsToGain(eqToDb(rLow)));
    auto rightMidCoeffs = juce::IIRCoefficients::makePeakFilter(currentSampleRate, 1000.0, 1.0,
                                                    juce::Decibels::decibelsToGain(eqToDb(rMid)));
    auto rightHighCoeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 4000.0, 0.7,
                                                    juce::Decibels::decibelsToGain(eqToDb(rHigh)));
    
    // Apply coefficients to filters
    for (int i = 0; i < 2; ++i) {
        leftLowFilter[i].setCoefficients(leftLowCoeffs);
        leftMidFilter[i].setCoefficients(leftMidCoeffs);
        leftHighFilter[i].setCoefficients(leftHighCoeffs);
        rightLowFilter[i].setCoefficients(rightLowCoeffs);
        rightMidFilter[i].setCoefficients(rightMidCoeffs);
        rightHighFilter[i].setCoefficients(rightHighCoeffs);
    }
}


//--- Audio callbacks --- //
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate) {
    // This is called before the audio device is started
    
    // Store audio settings for later use
    currentSampleRate = sampleRate;
    currentBufferSize = samplesPerBlockExpected;

    // Keep decks informed for loop math
    deckLeft.setSampleRate(sampleRate);
    deckRight.setSampleRate(sampleRate);

    // Prepare resampling sources
    if (resamplingSourceLeft != nullptr)
        resamplingSourceLeft->prepareToPlay(samplesPerBlockExpected, sampleRate);
    if (resamplingSourceRight != nullptr)
        resamplingSourceRight->prepareToPlay(samplesPerBlockExpected, sampleRate);
    
    // Prepare transport sources
    transportSourceLeft.prepareToPlay(samplesPerBlockExpected, sampleRate);
    transportSourceRight.prepareToPlay(samplesPerBlockExpected, sampleRate);
    
    // Prepare sample transports
    for (int i = 0; i < numSamplerPads; ++i) {
        sampleTransportLeft[i].prepareToPlay(samplesPerBlockExpected, sampleRate);
        sampleTransportRight[i].prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
    // Initialize EQ filters
    updateEQFilters();
}


// --- Audio processing callback --- //
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) {

    // This is called to fetch the next block of audio data
    if(readerSourceLeft == nullptr && readerSourceRight == nullptr) {
        bufferToFill.clearActiveBufferRegion(); // No audio to play
        return;
    }

    // Clear the buffer first
    bufferToFill.clearActiveBufferRegion();

    // Get current mixer values
    float leftVol = leftVolumeGain.load();
    float rightVol = rightVolumeGain.load();
    float crossfade = crossfaderPosition.load();
    float leftTrim = leftTrimGain.load();
    float rightTrim = rightTrimGain.load();
    
    // Calculate crossfader gains
    // Left deck: full volume when crossfader is left (0.0), silent when right (1.0)
    // Right deck: silent when crossfader is left (0.0), full volume when right (1.0)
    float leftCrossfadeGain = std::cos(crossfade * juce::MathConstants<float>::halfPi);
    float rightCrossfadeGain = std::sin(crossfade * juce::MathConstants<float>::halfPi);
    
    // Create a temporary buffer for mixing
    juce::AudioBuffer<float> tempBuffer(bufferToFill.buffer->getNumChannels(), 
                                        bufferToFill.numSamples);

    // Get audio from left deck and mix it in (through resampling source)
    if (resamplingSourceLeft != nullptr) {
        juce::AudioSourceChannelInfo leftInfo(&tempBuffer, 0, bufferToFill.numSamples);
        resamplingSourceLeft->getNextAudioBlock(leftInfo);
        
        // Apply trim gain before EQ
        tempBuffer.applyGain(0, bufferToFill.numSamples, leftTrim);
        
        // Apply EQ filters to left deck
        for (int channel = 0; channel < std::min(2, tempBuffer.getNumChannels()); ++channel) {
            leftLowFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
            leftMidFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
            leftHighFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
        }
        
        // Apply volume and crossfader gain
        float leftGain = leftVol * leftCrossfadeGain;
        
        // Add left deck to output buffer with gain applied
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {
            bufferToFill.buffer->addFrom(channel, bufferToFill.startSample, 
                                        tempBuffer, channel, 0, bufferToFill.numSamples, leftGain);
        }
    }

    // Get audio from right deck and mix it in (through resampling source)
    if (resamplingSourceRight != nullptr) {
        tempBuffer.clear();
        juce::AudioSourceChannelInfo rightInfo(&tempBuffer, 0, bufferToFill.numSamples);
        resamplingSourceRight->getNextAudioBlock(rightInfo);
        
        // Apply trim gain before EQ
        tempBuffer.applyGain(0, bufferToFill.numSamples, rightTrim);
        
        // Apply EQ filters to right deck
        for (int channel = 0; channel < std::min(2, tempBuffer.getNumChannels()); ++channel) {
            rightLowFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
            rightMidFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
            rightHighFilter[channel].processSamples(tempBuffer.getWritePointer(channel), bufferToFill.numSamples);
        }
        
        // Apply volume and crossfader gain
        float rightGain = rightVol * rightCrossfadeGain;
        
        // Add right deck to output buffer with gain applied
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {
            bufferToFill.buffer->addFrom(channel, bufferToFill.startSample, 
                                        tempBuffer, channel, 0, bufferToFill.numSamples, rightGain);
        }
    }
    
    // Mix in samples from left deck pads
    for (int i = 0; i < numSamplerPads; ++i) {
        if (sampleTransportLeft[i].isPlaying()) {
            tempBuffer.clear();
            juce::AudioSourceChannelInfo sampleInfo(&tempBuffer, 0, bufferToFill.numSamples);
            sampleTransportLeft[i].getNextAudioBlock(sampleInfo);
            
            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {
                bufferToFill.buffer->addFrom(channel, bufferToFill.startSample,
                                            tempBuffer, channel, 0, bufferToFill.numSamples);
            }
        }
    }
    
    // Mix in samples from right deck pads
    for (int i = 0; i < numSamplerPads; ++i) {
        if (sampleTransportRight[i].isPlaying()) {
            tempBuffer.clear();
            juce::AudioSourceChannelInfo sampleInfo(&tempBuffer, 0, bufferToFill.numSamples);
            sampleTransportRight[i].getNextAudioBlock(sampleInfo);
            
            for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel) {
                bufferToFill.buffer->addFrom(channel, bufferToFill.startSample,
                                            tempBuffer, channel, 0, bufferToFill.numSamples);
            }
        }
    }
}


//--- Audio release callback ---//
void MainComponent::releaseResources() {
    // called when the audio device stops or is being restarted
    if (resamplingSourceLeft != nullptr)
        resamplingSourceLeft->releaseResources();
    if (resamplingSourceRight != nullptr)
        resamplingSourceRight->releaseResources();
    
    // Release transport sources
    transportSourceLeft.releaseResources(); 
    transportSourceRight.releaseResources();
    
    // Release sample transports
    for (int i = 0; i < numSamplerPads; ++i) {
        sampleTransportLeft[i].releaseResources();
        sampleTransportRight[i].releaseResources();
    }
}


//--- Timer callback for GUI updates ---//
void MainComponent::timerCallback() {
    // Left waveform update
    waveformLeft.updatePlaybackPosition(transportSourceLeft.getCurrentPosition(), 
                    transportSourceLeft.getLengthInSeconds());

    // Right waveform update
    waveformRight.updatePlaybackPosition(transportSourceRight.getCurrentPosition(),
                    transportSourceRight.getLengthInSeconds());
}


void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(57, 57, 59));

    //--- Draw waveform backgrounds ---//
    // waveform separator line
    const int waveformSeparatorY = waveformLeft.getHeight();
    g.setColour(juce::Colour(70, 70, 75));
    g.drawLine(0.0f, (float)waveformSeparatorY, (float)getWidth(), (float)waveformSeparatorY, 3.0f);
}


void MainComponent::resized() {
    auto area = getLocalBounds();
    const int totalWidth = getWidth();

    //--- WAVEFORMS (TOP) ---//
    const int waveformHeight = 64;
    waveformLeft.setBounds(area.removeFromTop(waveformHeight));
    waveformRight.setBounds(area.removeFromTop(waveformHeight));
    
    waveformArea = juce::Rectangle<int>(0, 0, totalWidth, waveformHeight * 2);

    //--- MIXER CONTROLS (CENTER) ---//
    const int mixerWidth = 200;
    const int mixerHeight = 428;
    const int mixerX = (totalWidth - mixerWidth) / 2;
    const int mixerY = waveformArea.getBottom() + 12;

    mixerControls.setBounds(mixerX, mixerY, mixerWidth, mixerHeight);
    decksArea = juce::Rectangle<int>(mixerX, mixerY, mixerWidth, mixerHeight);

    //--- LEFT DECK (LEFT SIDE) ---//
    const int leftDeckX = MixerControls::calculateLeftDeckX();
    const int leftDeckY = waveformArea.getBottom() + 12;
    const int leftDeckWidth = MixerControls::calculateDeckWidth(totalWidth, mixerWidth);
    const int leftDeckHeight = 500;
    
    deckLeft.setBounds(leftDeckX, leftDeckY, leftDeckWidth, leftDeckHeight);

    //--- RIGHT DECK (RIGHT SIDE) ---//
    const int rightDeckX = MixerControls::calculateRightDeckX(totalWidth, mixerWidth);
    const int rightDeckY = waveformArea.getBottom() + 12;
    const int rightDeckWidth = MixerControls::calculateDeckWidth(totalWidth, mixerWidth);
    const int rightDeckHeight = 500;
    deckRight.setBounds(rightDeckX, rightDeckY, rightDeckWidth, rightDeckHeight);

    //--- PLAYLIST (BOTTOM) ---//
    const int playlistRowHeight = 36;
    const int numPlaylistRows = 4; // Show 4 rows + header (scrollbar appears at 5th song)
    const int playlistHeight = playlistRowHeight * (numPlaylistRows + 1); // +1 for header
    const int playlistY = mixerY + mixerHeight + 24;
    playlistHeader.setBounds(0, playlistY, totalWidth, playlistHeight);

    //--- ADD BUTTON (BOTTOM LEFT) ---//
    const int buttonSize = 40;
    const int buttonPadding = 12;
    const int buttonX = buttonPadding;
    const int buttonY = getHeight() - buttonSize - buttonPadding;
    addPlaylistButton.setBounds(buttonX, buttonY, buttonSize, buttonSize);
}


//--- Sampler Pad Functions ---//
void MainComponent::loadSampleToPad(int deckIndex, int padIndex, const juce::File& file) {
    // Validate pad index
    if (padIndex < 0 || padIndex >= numSamplerPads) return;
    
    // Create reader for the sample file
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr) {
        // Check duration (max 6 seconds)
        double duration = reader->lengthInSamples / reader->sampleRate;
        if (duration > 6.0) {
            delete reader;
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Sample Too Long",
                "Please select a sample shorter than 6 seconds.",
                "OK");
            return;
        }
        
        // Get appropriate arrays based on deck index
        auto* sampleReaders = (deckIndex == 0) ? sampleReaderLeft : sampleReaderRight;
        auto* sampleTransports = (deckIndex == 0) ? sampleTransportLeft : sampleTransportRight;
        
        // Clean up old sample if exists
        if (sampleReaders[padIndex] != nullptr) {
            clearSamplePad(deckIndex, padIndex);
        }
        
        // Create new reader source
        auto* newSource = new juce::AudioFormatReaderSource(reader, true);
        sampleReaders[padIndex] = newSource;
        sampleTransports[padIndex].setSource(newSource, 0, nullptr, reader->sampleRate);
        
        // Prepare if audio is already running
        if (currentSampleRate > 0) {
            sampleTransports[padIndex].prepareToPlay(currentBufferSize, currentSampleRate);
        }
    }
}


//--- Clear sample from a sampler pad ---//
void MainComponent::clearSamplePad(int deckIndex, int padIndex) {
    if (padIndex < 0 || padIndex >= numSamplerPads) {
        return;
    }

    auto* sampleReaders = (deckIndex == 0) ? sampleReaderLeft : sampleReaderRight;
    auto* sampleTransports = (deckIndex == 0) ? sampleTransportLeft : sampleTransportRight;

    sampleTransports[padIndex].stop();
    sampleTransports[padIndex].setSource(nullptr);

    if (sampleReaders[padIndex] != nullptr) {
        delete sampleReaders[padIndex];
        sampleReaders[padIndex] = nullptr;
    }
}


//--- Trigger sample playback ---//
void MainComponent::triggerSample(int deckIndex, int padIndex) {
    // Validate pad index
    if (padIndex < 0 || padIndex >= numSamplerPads) return;
    
    // Get appropriate transport array based on deck index
    auto* sampleTransports = (deckIndex == 0) ? sampleTransportLeft : sampleTransportRight;
    
    // Restart sample playback
    sampleTransports[padIndex].stop();
    sampleTransports[padIndex].setPosition(0.0);
    sampleTransports[padIndex].start();
}


//--- Open file chooser to add to playlist ---//
void MainComponent::openFileChooser() {
    auto chooserFlags = juce::FileBrowserComponent::openMode 
                    | juce::FileBrowserComponent::canSelectFiles
                    | juce::FileBrowserComponent::canSelectMultipleItems;
    
    playlistFileChooser = std::make_unique<juce::FileChooser>("Select audio files to add to playlist",
                                                juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                "*.mp3;*.wav;*.flac;*.m4a;*.aif;*.aiff;*.ogg");
    
    playlistFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        auto selectedFiles = chooser.getResults();
        
        if (selectedFiles.isEmpty()) return; // User cancelled
        
        // Process all selected files
        for (const auto& selectedFile : selectedFiles) {
            processAndAddFileToPlaylist(selectedFile);
        }
    });
}


//--- Process and add file to playlist ---//
void MainComponent::processAndAddFileToPlaylist(const juce::File& file) {
    // Read metadata from file
    auto* reader = formatManager.createReaderFor(file);
    if (reader != nullptr) {
        // Extract basic info
        double lengthInSeconds = reader->lengthInSamples / reader->sampleRate;
        
        // Try to get metadata (title, artist, BPM)
        juce::String title = file.getFileNameWithoutExtension();
        juce::String artist = "Unknown";
        int bpm = 120; // Default BPM
        
        // Analyze for BPM if available
        AudioAnalyzer::AnalysisResult result = audioAnalyzer.analyzeFile(file, formatManager);
        if (result.bpmDetected) {
            bpm = static_cast<int>(result.bpm);
        }
        
        // Create playlist song and add
        PlaylistSong song;
        song.title = title;
        song.artist = artist;
        song.lengthSeconds = static_cast<int>(lengthInSeconds);
        song.bpm = bpm;
        song.file = file;
        
        playlistHeader.addSong(song);
        
        delete reader;
    }
}