/*
    Playlist.h
    Playlist handles the display and management of the song list, including drag-and-drop support, song metadata display,
    and interactions for loading songs to decks or removing them from the playlist.
*/


#pragma once
#include <JuceHeader.h>


//--- Layout constants ---//
namespace PlaylistLayout {
    // Dimensions
    constexpr int ROW_HEIGHT = 36;
    constexpr int HEADER_HEIGHT = 36;
    constexpr int LABEL_FONT_SIZE = 12;
    constexpr int HEADER_FONT_SIZE = 14;
    constexpr int LABEL_TEXT_HEIGHT = 19;
    constexpr int LABEL_VERTICAL_PADDING = (ROW_HEIGHT - LABEL_TEXT_HEIGHT) / 2;
    
    // Column widths
    constexpr int COLUMN_NUMBER_WIDTH = 32;
    constexpr int COLUMN_TITLE_WIDTH = 348;
    constexpr int COLUMN_ARTIST_WIDTH = 112;
    constexpr int COLUMN_TIME_WIDTH = 90;
    constexpr int COLUMN_BPM_WIDTH = 64;
    constexpr int COLUMN_LEFT_DECK_WIDTH = 80;
    constexpr int COLUMN_RIGHT_DECK_WIDTH = 80;
    constexpr int COLUMN_REMOVE_WIDTH = 80;
    
    // Spacing between columns
    constexpr int SPACING_SMALL = 40;
    constexpr int SPACING_LARGE = 140;
    constexpr int SPACING_BUTTONS = 32;
    
    // Button padding
    constexpr int BUTTON_ICON_PADDING = 10;
    
    // Colors
    const juce::Colour BACKGROUND_NORMAL {juce::Colours::transparentBlack};
    const juce::Colour BACKGROUND_HOVER {juce::Colour(69, 70, 75)};
    const juce::Colour BACKGROUND_HEADER {juce::Colour(32, 32, 35)};
    const juce::Colour BACKGROUND_ROWS {juce::Colour(57, 57, 59)};
    const juce::Colour TEXT_COLOUR {juce::Colour(215, 215, 215)};
    const juce::Colour STROKE_COLOUR {juce::Colour(45, 46, 47)};
    const juce::Colour ICON_COLOUR {juce::Colour(236, 236, 236)};
}

//--- SVG icons ---//
namespace PlaylistIcons {
    constexpr const char* DECK_ARROW_SVG = R"(
    <svg xmlns="http://www.w3.org/2000/svg" width="12" height="14" viewBox="0 0 32 32" fill="none">
    <path d="M10 10.5556V9H22V10.5556H10ZM15.25 23V15.0667L13.3 17.0889L12.25 16L16 12.1111L19.75 16L18.7 17.0889L16.75 15.0667V23H15.25Z" fill="#ECECEC"/>
    </svg>
    )";
    
    constexpr const char* TRASH_CAN_SVG = R"(
    <svg xmlns="http://www.w3.org/2000/svg" width="12" height="14" viewBox="0 0 32 32" fill="none">
    <path d="M12.25 23C11.8375 23 11.4844 22.8477 11.1906 22.5431C10.8969 22.2384 10.75 21.8722 10.75 21.4444V11.3333H10V9.77778H13.75V9H18.25V9.77778H22V11.3333H21.25V21.4444C21.25 21.8722 21.1031 22.2384 20.8094 22.5431C20.5156 22.8477 20.1625 23 19.75 23H12.25ZM19.75 11.3333H12.25V21.4444H19.75V11.3333ZM13.75 19.8889H15.25V12.8889H13.75V19.8889ZM16.75 19.8889H18.25V12.8889H16.75V19.8889Z" fill="#ECECEC"/>
    </svg>
    )";
}


//--- Playlist song data ---//
struct PlaylistSong {
    int index;              // Position in playlist (1, 2, 3...)
    juce::String title;
    juce::String artist;
    double lengthSeconds;   // Song duration in seconds
    double bpm;
    juce::File file;        // Original file reference
};


//--- PlaylistRow component ---//
class PlaylistRow : public juce::Component {
    public:
        // Purpose: Create a playlist row for a song.
        // Inputs: song data.
        // Outputs: initialized PlaylistRow instance.
        PlaylistRow(const PlaylistSong& song);
        // Purpose: Release row resources.
        // Inputs: none.
        // Outputs: none.
        ~PlaylistRow() override = default;

        // Purpose: Render the row UI.
        // Inputs: graphics context.
        // Outputs: none.
        void paint(juce::Graphics& g) override;
        // Purpose: Layout row controls.
        // Inputs: none.
        // Outputs: none.
        void resized() override;
        // Purpose: Handle mouse entering the row.
        // Inputs: mouse event.
        // Outputs: none.
        void mouseEnter(const juce::MouseEvent& event) override;
        // Purpose: Handle mouse exiting the row.
        // Inputs: mouse event.
        // Outputs: none.
        void mouseExit(const juce::MouseEvent& event) override;

        // Callbacks for button interactions
        std::function<void(int)> onLeftDeckClicked;
        std::function<void(int)> onRightDeckClicked;
        std::function<void(int)> onRemoveClicked;

        // Purpose: Update the row with new song data.
        // Inputs: updated song data.
        // Outputs: none.
        void updateSong(const PlaylistSong& song);
        
        PlaylistSong song;  // Current song data public for access by Playlist

    private:
        bool isHovered {false};

        juce::Label numberLabel;
        juce::Label titleLabel;
        juce::Label artistLabel;
        juce::Label timeLabel;
        juce::Label bpmLabel;

        // Custom button that forwards hover to parent row
        class RowButton : public juce::DrawableButton {
        public:
            RowButton(const juce::String& name, PlaylistRow* parent) 
                : juce::DrawableButton(name, ImageFitted), parentRow(parent) {}
            
            void mouseEnter(const juce::MouseEvent& e) override {
                juce::DrawableButton::mouseEnter(e);
                if (parentRow) parentRow->mouseEnter(e);
            }
            
            void mouseExit(const juce::MouseEvent& e) override {
                juce::DrawableButton::mouseExit(e);
                if (parentRow) parentRow->mouseExit(e);
            }

        private:
            PlaylistRow* parentRow;
        };

        // Buttons
        RowButton leftDeckButton {"leftDeck", this};
        RowButton rightDeckButton {"rightDeck", this};
        RowButton removeButton {"remove", this};

        // SVG Drawables
        std::unique_ptr<juce::Drawable> deckIconDrawable;
        std::unique_ptr<juce::Drawable> removeIconDrawable;

        // Helper methods
        juce::String formatTime(double seconds) const;
        void configureLabel(juce::Label& label, const juce::String& text, juce::Justification just);
        void configureDeckButton(RowButton& btn);
        void configureRemoveButton(RowButton& btn);
        std::unique_ptr<juce::Drawable> createDrawableFromSVG(const char* svgData);
};


//--- Playlist component ---//
class Playlist : public juce::Component,
                    public juce::FileDragAndDropTarget {
    public:
    // Purpose: Create the playlist component.
    // Inputs: none.
    // Outputs: initialized Playlist instance.
    Playlist();
    // Purpose: Release playlist resources.
    // Inputs: none.
    // Outputs: none.
    ~Playlist() override = default;

    // Purpose: Render the playlist UI.
    // Inputs: graphics context.
    // Outputs: none.
        void paint(juce::Graphics& g) override;
    // Purpose: Layout playlist controls.
    // Inputs: none.
    // Outputs: none.
        void resized() override;
        
    // Purpose: Determine if files can be dragged over the playlist.
    // Inputs: dragged file list.
    // Outputs: true if interested.
        bool isInterestedInFileDrag(const juce::StringArray& files) override;
    // Purpose: Handle dropped files.
    // Inputs: file list, drop position x/y.
    // Outputs: none.
        void filesDropped(const juce::StringArray& files, int x, int y) override;
    // Purpose: Handle drag enter events.
    // Inputs: file list, position x/y.
    // Outputs: none.
        void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    // Purpose: Handle drag exit events.
    // Inputs: file list.
    // Outputs: none.
        void fileDragExit(const juce::StringArray& files) override;
        
    // Purpose: Add a song to the playlist.
    // Inputs: song data.
    // Outputs: none.
        void addSong(const PlaylistSong& song);
    // Purpose: Remove a song by index.
    // Inputs: song index.
    // Outputs: none.
        void removeSong(int index);
    // Purpose: Clear all songs from the playlist.
    // Inputs: none.
    // Outputs: none.
        void clearPlaylist();
        
    // Purpose: Save playlist state to disk.
    // Inputs: none.
    // Outputs: none.
        void savePlaylist();
    // Purpose: Load playlist state from disk.
    // Inputs: none.
    // Outputs: none.
        void loadPlaylist();
        
        // Callbacks for deck loading
        std::function<void(const juce::File&)> onLoadToLeftDeck;
        std::function<void(const juce::File&)> onLoadToRightDeck;
        
        // Callback for processing dropped files
        std::function<void(const juce::File&)> onFileDropped;

    private:
        // Header labels
        juce::Label numberLabel;
        juce::Label titleLabel;
        juce::Label artistLabel;
        juce::Label timeLabel;
        juce::Label bpmLabel;
        juce::Label leftDeckLabel;
        juce::Label rightDeckLabel;
        juce::Label removeLabel;

        // Rows management
        juce::OwnedArray<PlaylistRow> rows;
        
        // Scrollable container for rows
        class RowsContainer : public juce::Component {
        public:
            juce::OwnedArray<PlaylistRow>* rows = nullptr;
            void resized() override {
                int positionY = 0;
                if (rows) {
                    for (auto* row : *rows) {
                        row->setBounds(0, positionY, getWidth(), PlaylistLayout::ROW_HEIGHT);
                        positionY += PlaylistLayout::ROW_HEIGHT;
                    }
                }
            }
        };
        
        RowsContainer rowsContainer;
        juce::Viewport viewport;
        
        // Drag-and-drop state
        bool isDragOver = false;
        
        // Helper methods
        juce::File getPlaylistFile();
        void updateRowNumbers();
        void configureLabel(juce::Label& label, const juce::String& text, juce::Justification just);
        bool isAudioFile(const juce::String& filename);
};