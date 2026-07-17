/*
    Playlist.cpp
    Manages the playlist component, including song rows, drag-and-drop, and persistence.
    Main responsibilities:
    - Render playlist header and rows
    - Handle adding/removing songs
    - Support drag-and-drop of audio files
    - Support loading multiple songs at once
    - Save/load playlist to/from disk
*/


#include "Playlist.h"


//--- PlaylistRow implementation ---//
PlaylistRow::PlaylistRow(const PlaylistSong& song) : song(song) {
    // Create SVG drawables from constants
    deckIconDrawable = createDrawableFromSVG(PlaylistIcons::DECK_ARROW_SVG);
    removeIconDrawable = createDrawableFromSVG(PlaylistIcons::TRASH_CAN_SVG);
    
    updateSong(song);
}


//--- PlaylistRow methods ---//
void PlaylistRow::updateSong(const PlaylistSong& newSong) {
    this->song = newSong;
    
    configureLabel(numberLabel, juce::String(newSong.index).paddedLeft('0', 3), juce::Justification::centred);
    configureLabel(titleLabel, newSong.title, juce::Justification::left);
    configureLabel(artistLabel, newSong.artist, juce::Justification::left);
    configureLabel(timeLabel, formatTime(newSong.lengthSeconds), juce::Justification::left);
    configureLabel(bpmLabel, juce::String((int)newSong.bpm), juce::Justification::centred);
    
    configureDeckButton(leftDeckButton);
    configureDeckButton(rightDeckButton);
    configureRemoveButton(removeButton);
    
    // Setup button callbacks with current song index
    leftDeckButton.onClick = [this, newSong]() {
        if (onLeftDeckClicked) onLeftDeckClicked(newSong.index);
    };
    
    rightDeckButton.onClick = [this, newSong]() {
        if (onRightDeckClicked) onRightDeckClicked(newSong.index);
    };
    
    removeButton.onClick = [this, newSong]() {
        if (onRemoveClicked) onRemoveClicked(newSong.index);
    };
}

//--- PlaylistRow mouse events ---//
void PlaylistRow::paint(juce::Graphics& g) {
    // Background
    g.fillAll(isHovered ? PlaylistLayout::BACKGROUND_HOVER : PlaylistLayout::BACKGROUND_NORMAL);
    
    // Bottom stroke line
    g.setColour(PlaylistLayout::STROKE_COLOUR);
    g.drawLine(0.0f, (float)(getHeight() - 1), (float)getWidth(), (float)(getHeight() - 1), 1.0f);
}


void PlaylistRow::resized() {
    const int positionY = PlaylistLayout::LABEL_VERTICAL_PADDING;
    int positionX = 0;

    // Position labels and buttons according to defined layout.
    numberLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_NUMBER_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_NUMBER_WIDTH + PlaylistLayout::SPACING_SMALL;

    titleLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_TITLE_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_TITLE_WIDTH + PlaylistLayout::SPACING_SMALL;

    artistLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_ARTIST_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_ARTIST_WIDTH + PlaylistLayout::SPACING_SMALL;

    timeLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_TIME_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_TIME_WIDTH + PlaylistLayout::SPACING_SMALL;

    bpmLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_BPM_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_BPM_WIDTH + PlaylistLayout::SPACING_LARGE;

    // Buttons span full row height, icons auto-center with ImageFitted mode
    leftDeckButton.setBounds(positionX, 0, PlaylistLayout::COLUMN_LEFT_DECK_WIDTH, PlaylistLayout::ROW_HEIGHT);
    positionX += PlaylistLayout::COLUMN_LEFT_DECK_WIDTH + PlaylistLayout::SPACING_BUTTONS;

    rightDeckButton.setBounds(positionX, 0, PlaylistLayout::COLUMN_RIGHT_DECK_WIDTH, PlaylistLayout::ROW_HEIGHT);
    positionX += PlaylistLayout::COLUMN_RIGHT_DECK_WIDTH + PlaylistLayout::SPACING_BUTTONS;

    removeButton.setBounds(positionX, 0, PlaylistLayout::COLUMN_REMOVE_WIDTH, PlaylistLayout::ROW_HEIGHT);
}

//--- PlaylistRow mouse events ---//
void PlaylistRow::mouseEnter(const juce::MouseEvent& /*event*/) {
    isHovered = true;
    repaint();
}

//--- PlaylistRow mouse events ---//
void PlaylistRow::mouseExit(const juce::MouseEvent& /*event*/) {
    isHovered = false;
    repaint();
}

//--- PlaylistRow helpers ---//
juce::String PlaylistRow::formatTime(double seconds) const {
    int mins = (int)seconds / 60;
    int secs = (int)seconds % 60;
    return juce::String::formatted("%d:%02d", mins, secs);
}

//--- PlaylistRow helpers ---//
void PlaylistRow::configureLabel(juce::Label& label, const juce::String& text, juce::Justification just) {
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(just);
    label.setFont(juce::FontOptions((float)PlaylistLayout::LABEL_FONT_SIZE));
    label.setColour(juce::Label::textColourId, PlaylistLayout::TEXT_COLOUR);
    label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label.setInterceptsMouseClicks(false, false); // Allow mouse events to pass through to row
    addAndMakeVisible(label);
}

//--- PlaylistRow helpers ---//
void PlaylistRow::configureDeckButton(RowButton& btn) {
    if (deckIconDrawable) {
        auto normal = deckIconDrawable->createCopy();
        auto over = deckIconDrawable->createCopy();
        btn.setImages(normal.release(), over.release(), over.release(), nullptr, nullptr, nullptr, nullptr);
    }
    btn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    btn.setClickingTogglesState(false);
    btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    btn.setEdgeIndent(PlaylistLayout::BUTTON_ICON_PADDING);
    addAndMakeVisible(btn);
}

//--- PlaylistRow helpers ---//
void PlaylistRow::configureRemoveButton(RowButton& btn) {
    if (removeIconDrawable) {
        auto normal = removeIconDrawable->createCopy();
        auto over = removeIconDrawable->createCopy();
        btn.setImages(normal.release(), over.release(), over.release(), nullptr, nullptr, nullptr, nullptr);
    }
    btn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    btn.setClickingTogglesState(false);
    btn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    btn.setEdgeIndent(PlaylistLayout::BUTTON_ICON_PADDING);
    addAndMakeVisible(btn);
}

//--- PlaylistRow helpers ---//
std::unique_ptr<juce::Drawable> PlaylistRow::createDrawableFromSVG(const char* svgData) {
    juce::MemoryInputStream stream(svgData, std::strlen(svgData), false);
    auto svgXml = juce::XmlDocument::parse(stream.readEntireStreamAsString());
    if (svgXml != nullptr)
        return juce::Drawable::createFromSVG(*svgXml);
    return {};
}


//--- Playlist implementation ---//
Playlist::Playlist() {
    configureLabel(numberLabel, "#", juce::Justification::centred);
    configureLabel(titleLabel, "Title", juce::Justification::left);
    configureLabel(artistLabel, "Artist", juce::Justification::left);
    configureLabel(timeLabel, "Time", juce::Justification::left);
    configureLabel(bpmLabel, "BPM", juce::Justification::centred);
    configureLabel(leftDeckLabel, "Left Deck", juce::Justification::centred);
    configureLabel(rightDeckLabel, "Right Deck", juce::Justification::centred);
    configureLabel(removeLabel, "Remove", juce::Justification::centred);
    
    // Setup viewport for scrolling rows
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&rowsContainer, false);
    viewport.setScrollBarsShown(true, false);
    rowsContainer.rows = &rows;
}


void Playlist::paint(juce::Graphics& g) {
    // Paint header background
    g.setColour(PlaylistLayout::BACKGROUND_HEADER);
    g.fillRect(0, 0, getWidth(), PlaylistLayout::HEADER_HEIGHT);
    
    // Paint rows background
    g.setColour(PlaylistLayout::BACKGROUND_ROWS);
    g.fillRect(0, PlaylistLayout::HEADER_HEIGHT, getWidth(), getHeight() - PlaylistLayout::HEADER_HEIGHT);
    
    // Draw drag-over highlight
    if (isDragOver) {
        g.setColour(juce::Colour(100, 150, 255).withAlpha(0.3f));
        g.fillRect(getLocalBounds());
        
        g.setColour(juce::Colour(100, 150, 255));
        g.drawRect(getLocalBounds(), 2);
        
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Drop audio files here", getLocalBounds(), juce::Justification::centred);
    }
}


void Playlist::resized() {
    auto area = getLocalBounds();
    
    // Header row (fixed at top)
    auto headerArea = area.removeFromTop(PlaylistLayout::HEADER_HEIGHT);
    const int positionY = PlaylistLayout::LABEL_VERTICAL_PADDING;
    int positionX = 0;

    numberLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_NUMBER_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_NUMBER_WIDTH + PlaylistLayout::SPACING_SMALL;

    titleLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_TITLE_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_TITLE_WIDTH + PlaylistLayout::SPACING_SMALL;

    artistLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_ARTIST_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_ARTIST_WIDTH + PlaylistLayout::SPACING_SMALL;

    timeLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_TIME_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_TIME_WIDTH + PlaylistLayout::SPACING_SMALL;

    bpmLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_BPM_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_BPM_WIDTH + PlaylistLayout::SPACING_LARGE;

    leftDeckLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_LEFT_DECK_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_LEFT_DECK_WIDTH + PlaylistLayout::SPACING_BUTTONS;

    rightDeckLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_RIGHT_DECK_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);
    positionX += PlaylistLayout::COLUMN_RIGHT_DECK_WIDTH + PlaylistLayout::SPACING_BUTTONS;

    removeLabel.setBounds(positionX, positionY, PlaylistLayout::COLUMN_REMOVE_WIDTH, PlaylistLayout::LABEL_TEXT_HEIGHT);

    // Viewport for scrollable rows
    viewport.setBounds(area);
    
    // Update rows container size
    int totalRowsHeight = rows.size() * PlaylistLayout::ROW_HEIGHT;
    rowsContainer.setSize(area.getWidth(), totalRowsHeight);
}


//--- Add songs ---//
void Playlist::addSong(const PlaylistSong& song) {

    // Ensure the song has the correct index
    PlaylistSong updatedSong = song;
    updatedSong.index = rows.size() + 1;
    
    auto* newRow = new PlaylistRow(updatedSong); // Create new row
    
    // Setup callbacks
    newRow->onLeftDeckClicked = [this](int index) {
        if (index > 0 && index <= rows.size()) {
            auto* row = rows[index - 1];
            if (row && onLoadToLeftDeck) {
                onLoadToLeftDeck(row->song.file);
            }
        }
    };
    
    newRow->onRightDeckClicked = [this](int index) {
        if (index > 0 && index <= rows.size()) {
            auto* row = rows[index - 1];
            if (row && onLoadToRightDeck) {
                onLoadToRightDeck(row->song.file);
            }
        }
    };
    
    newRow->onRemoveClicked = [this](int index) {
        if (index > 0 && index <= rows.size()) {
            removeSong(index - 1); // Convert 1-based index to 0-based
        }
    };
    
    rows.add(newRow);
    rowsContainer.addAndMakeVisible(newRow);
    resized();
    savePlaylist(); // Auto-save after adding
}

//--- Remove songs ---//
void Playlist::removeSong(int index) {
    if (index >= 0 && index < rows.size()) {
        rows.remove(index);
        updateRowNumbers();
        resized();
        savePlaylist(); // Auto-save after removing
    }
}

//--- Clear playlist ---//
void Playlist::clearPlaylist() {
    rows.clear();
    resized();
    savePlaylist(); // Auto-save after clearing
}

//--- Update row numbers ---//
void Playlist::updateRowNumbers() {
    for (int i = 0; i < rows.size(); ++i) {
        PlaylistSong song = rows[i]->song;
        song.index = i + 1;
        rows[i]->updateSong(song);
    }
}

//--- Configure header labels ---//
void Playlist::configureLabel(juce::Label& label, const juce::String& text, juce::Justification just) {
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(just);
    label.setFont(juce::FontOptions((float)PlaylistLayout::HEADER_FONT_SIZE));
    label.setColour(juce::Label::textColourId, PlaylistLayout::TEXT_COLOUR);
    label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(label);
}

//--- Playlist persistence ---//
juce::File Playlist::getPlaylistFile() {
    // Store playlist in user's documents folder
    auto docsFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    return docsFolder.getChildFile("DJ App").getChildFile("playlist.json");
}

//--- Save playlist ---//
void Playlist::savePlaylist() {
    juce::var playlistArray;
    playlistArray = juce::var(juce::Array<juce::var>());
    
    for (int i = 0; i < rows.size(); ++i) {
        const auto& song = rows[i]->song;
        
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("title", song.title);
        obj->setProperty("artist", song.artist);
        obj->setProperty("length", song.lengthSeconds);
        obj->setProperty("bpm", song.bpm);
        obj->setProperty("path", song.file.getFullPathName());
        
        playlistArray.append(juce::var(obj.get()));
    }
    
    juce::File playlistFile = getPlaylistFile();
    playlistFile.getParentDirectory().createDirectory();
    
    juce::String jsonText = juce::JSON::toString(playlistArray, true);
    playlistFile.replaceWithText(jsonText);
}

//--- Load playlist ---//
void Playlist::loadPlaylist() {
    juce::File playlistFile = getPlaylistFile();
    
    if (!playlistFile.exists()) {
        return; // No saved playlist yet
    }
    
    juce::String jsonText = playlistFile.loadFileAsString();
    juce::var parsedData = juce::JSON::parse(jsonText);
    
    if (!parsedData.isArray()) {
        return;
    }
    
    clearPlaylist();
    
    for (int i = 0; i < parsedData.size(); ++i) {
        const auto& songObj = parsedData[i];
        
        if (!songObj.isObject()) {
            continue;
        }
        
        juce::File songFile(songObj.getProperty("path", "").toString());
        
        // Only add if file still exists
        if (songFile.existsAsFile()) {
            PlaylistSong song;
            song.title = songObj.getProperty("title", "Unknown").toString();
            song.artist = songObj.getProperty("artist", "Unknown").toString();
            song.lengthSeconds = (double)songObj.getProperty("length", 0.0);
            song.bpm = (double)songObj.getProperty("bpm", 0.0);
            song.file = songFile;
            
            addSong(song);
        }
    }
}

//--- Drag-and-drop overrides ---//

//--- Check if files are supported audio ---//
bool Playlist::isInterestedInFileDrag(const juce::StringArray& files) {
    // Check if any of the files are audio files
    for (const auto& file : files) {
        if (isAudioFile(file)) {
            return true;
        }
    }
    return false;
}

//--- Handle files being dropped ---//
void Playlist::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) {
    isDragOver = false;
    repaint();
    
    // Process each dropped file
    for (const auto& filePath : files) {
        juce::File file(filePath);
        if (file.existsAsFile() && isAudioFile(filePath)) {
            if (onFileDropped) {
                onFileDropped(file);
            }
        }
    }
}

//--- Handle drag enter ---//
void Playlist::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/) {
    isDragOver = true;
    repaint();
}

//--- Handle drag exit ---//
void Playlist::fileDragExit(const juce::StringArray& /*files*/) {
    isDragOver = false;
    repaint();
}

//--- Check if file is audio ---//
bool Playlist::isAudioFile(const juce::String& filename) {
    static const juce::StringArray audioExtensions = {".mp3", ".wav", ".flac", ".m4a", ".aif", ".aiff", ".ogg"};
    
    for (const auto& ext : audioExtensions) {
        if (filename.endsWithIgnoreCase(ext)) {
            return true;
        }
    }
    return false;
}