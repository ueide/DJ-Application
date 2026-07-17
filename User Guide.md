# DJ App DJ Application - User Guide

## Overview
DJ App is a professional dual-deck DJ application built with JUCE framework. This guide explains all features and controls for effective use in live mixing and music performance.

================================================================================

## Installation & Setup

### System Requirements
- **Operating System**: Windows 10/11, macOS, or Linux
- **Audio**: ASIO-compatible audio interface recommended for low latency
- **Storage**: User data stored in `Documents/DJ App/` folder
  - `playlist.json` - Music library data
  - `hotcues.json` - Per-track cue points
  - `mixer.json` - Per-track mixer settings

### Supported Audio Formats
- MP3, WAV, FLAC, M4A, AIF, AIFF, OGG

================================================================================

## Main Interface Layout
┌────────────────────────────────────────────────────────┐
│                  LEFT DECK WAVEFORM                    │
│                  RIGHT DECK WAVEFORM                   │
├───────────────┬─────────────────────┬──────────────────┤
│   LEFT DECK   │    MIXER CONTROLS   │    RIGHT DECK    │
└───────────────┴─────────────────────┴──────────────────┘
│                   PLAYLIST LIBRARY                     │
└────────────────────────────────────────────────────────┘


================================================================================

## DECK CONTROLS (Left & Right)


### Playback Controls

#### **PLAY/PAUSE Button**
- **Click**: Start or pause playback
- **Behavior**: Green when playing, gray when paused
- Independent for each deck

#### **CUE Button**
- **Click (while stopped)**: Set cue point to current position
- **Click (while playing)**: Return to cue point and pause
- **Hold (while stopped)**: Preview play from cue point (returns to cue on release)
- **Hold (while playing)**: Return to cue point, preview plays while held, resumes playback on release
- **Usage**: Essential for beatmatching and live transitions
- **Visual**: Orange/red when active

#### **LOAD Button**
- **Click**: Opens file browser to load track into this deck
- **Supports**: All audio formats listed above
- **Behavior**: Automatically analyzes BPM and loads waveform


### Tempo & Sync Controls

#### **TEMPO Slider** (Vertical Fader)
- **Range**: 50% to 200% speed
- **Default**: 100% (original tempo)
- **Usage**: Drag up to speed up, down to slow down
- **Real-time**: BPM display updates to show current tempo
- **Pitch**: Maintains pitch while adjusting speed (time-stretching)

#### **SYNC Button**
- **Click**: Automatically matches this deck's BPM to the opposite deck
- **Calculation**: Adjusts tempo slider based on BPM ratio
- **Usage**: Quick beatmatching for fast mixing
- **Visual**: Flashes or highlights when active


### Jog Wheel

#### **Mouse Interactions**
- **Click & Drag (outer ring)**: Scrub through track (vinyl-style)
- **Click & Drag (while playing)**: Nudge tempo for fine beatmatching
- **Click & Drag (while paused)**: Seek to precise position
- **Sensitivity**: Proportional to drag distance
- **Usage**: Fine tempo adjustments and track cueing

### Metadata Display
- **Track Title**: File name without extension
- **Artist**: "Unknown" if no metadata
- **BPM**: Detected or adjusted tempo value
- **Duration**: Total track length


================================================================================

## HOT CUE SYSTEM (8 Pads per Deck)

### Mode Selection
- **HOT CUE Mode** (Default): Set and trigger cue points
- **SAMPLER Mode**: Load and trigger one-shot samples

### Hot Cue Pad Interactions (HOT CUE Mode)

#### **Empty Pad (No Cue Assigned)**
- **Click**: Set cue point at current playback position
- **Visual**: Pad lights up (e.g., orange/yellow)
- **Waveform**: Blue numbered rectangle appears at cue position

#### **Active Pad (Cue Assigned)**
- **Click**: Jump playback to cue position instantly
- **Shift + Click**: **CLEAR** the cue (pad becomes empty)
- **Visual**: Pad color indicates cue is set
- **Usage**: Essential for live remixing, drops, vocal hits

#### **CLEAR ALL Button**
- **Location**: Below hot cue pad grid (right-aligned)
- **Click**: Removes ALL 8 cues for current deck
- **Confirmation**: Immediate action (use carefully!)
- **Usage**: Quick reset when loading new track for preparation

### Waveform Cue Markers
- **Visual**: Blue rectangles with pad numbers (1-8)
- **Size**: 12x20 pixels
- **Position**: Exact playback time of cue point
- **Zoom**: Markers scale with waveform zoom levels
- **Click**: Click marker to jump to that cue


================================================================================

## SAMPLER PADS (8 Pads per Deck)

### Sampler Mode Activation
- **HOT CUE/SAMPLER Toggle**: Switch between modes

### Sampler Pad Interactions

#### **Load Sample**
- **Click Empty Pad**: Opens file browser for sample
- **Formats**: Same as track formats (short samples recommended)
- **Storage**: Samples remain loaded until replaced

#### **Trigger Sample**
- **Click Loaded Pad**: Plays sample from beginning (one-shot)
- **Behavior**: Play on top of deck audio (non-stopping)
- **Multiple**: All 8 pads can play simultaneously
- **Usage**: Effects, vocals, drum hits, air horns


================================================================================

## MIXER CONTROLS (Center Section)

### Trim Controls (Top Knobs)

#### **TRIM Knob (Left & Right)**
- **Function**: Input gain adjustment
- **Range**: ±12dB
- **Default**: 0dB (center position)
- **Usage**: Level-match tracks with different recording volumes
- **Visual**: Rotary dial with +/- indicators
- **Behavior**: Adjusts gain BEFORE EQ processing


### 3-Band Equalizer (Per Deck)

#### **HIGH Knob** (>4kHz frequencies)
- **Range**: ±12dB boost/cut
- **Default**: 0dB (center/12 o'clock position)
- **Rotation**: Clockwise boosts, counter-clockwise cuts
- **Usage**: Control cymbals, hi-hats, bright vocals
- **DJ Technique**: Cut highs to remove clashing cymbals during transitions

#### **MID Knob** (400Hz - 4kHz frequencies)
- **Range**: ±12dB boost/cut
- **Default**: 0dB (center position)
- **Usage**: Control vocals, snares, most melodic content
- **DJ Technique**: Cut mids to create space for incoming track

#### **LOW Knob** (<400Hz frequencies)
- **Range**: ±12dB boost/cut
- **Default**: 0dB (center position)
- **Usage**: Control kick drums, bass lines
- **DJ Technique**: Swap bass between decks (cut low on one while boosting other)


### Volume Controls

#### **VOLUME Fader** (Vertical Fader)
- **Range**: 0.0 to 1.5 (0% to 150%)
- **Default**: 0.75 (75%)
- **Steps**: 15% increments
- **Usage**: Per-deck volume control
- **Drag**: Up to increase, down to decrease

#### **CROSSFADER** (Horizontal Fader)
- **Position**: Bottom of mixer section
- **Range**: Full left to full right
- **Default**: Center (both decks at equal volume)
- **Curve**: Equal-power panning (no energy loss in center)
- **Left Position**: Only left deck audible
- **Right Position**: Only right deck audible
- **Center Position**: Both decks at equal volume
- **Usage**: Smooth transitions between tracks


================================================================================

## WAVEFORM DISPLAYS (Top Section)

### Visual Elements

#### **Waveform**
- **Display**: Stereo audio amplitude over time
- **Color**: Blue/cyan gradient
- **Zoom**: Adjustable horizontal and vertical scale

#### **Playhead** (Red Vertical Line)
- **Position**: Current playback position
- **Movement**: Scrolls left to right during playback

#### **Beat Grid** (Vertical White Lines)
- **Source**: BPM detection algorithm
- **Spacing**: Quarter-note beats
- **Usage**: Visual alignment for beatmatching

#### **Loop Overlay** (Colored Rectangle)
- **Visible**: When loop is active
- **Color**: Semi-transparent (e.g., green/yellow)
- **Boundaries**: Loop start and end points

#### **Hot Cue Markers** (Blue Numbered Rectangles)
- **Numbers**: 1-8 corresponding to pad numbers
- **Position**: Exact cue point locations
- **Size**: 12x20 pixels
- **Interaction**: Click to jump to cue


### Waveform Interactions

#### **Click to Seek**
- **Click Anywhere**: Jump playback to clicked position
- **Calculation**: Time = (clickX / waveformWidth) * trackDuration
- **Usage**: Quick navigation through track

#### **Drag to Seek**
- **Click & Hold**: Scrub through track
- **Visual**: Playhead follows mouse position
- **Audio**: Scrubbing sound during drag

#### **Zoom Controls**
- **Vertical Zoom**: Adjust amplitude scale
- **Horizontal Zoom**: Adjust time scale (zoom in/out)
- **Mouse Wheel**: Zoom in/out (if implemented)
- **Usage**: Detailed view for precise cue setting


================================================================================

## LOOP CONTROLS

### Beat-Quantized Loop Lengths

#### **Loop Length Buttons**
- **1/4**: Quarter-beat loop
- **1**: One-beat loop
- **2**: Two-beat loop
- **4**: Four-beat loop
- **8**: Eight-beat loop

#### **Loop Activation**
- **Click Length Button**: Activate loop at current position
- **Calculation**: Length based on detected BPM
- **Visual**: Waveform overlay shows loop region
- **Playback**: Seamlessly repeats loop region

#### **Loop Deactivation**
- **Click Active Loop Button**: Deactivate loop
- **Behavior**: Playback continues from loop point
- **Usage**: Extend breakdowns, create build-ups


================================================================================

## PLAYLIST LIBRARY (Bottom Section)

### Adding Tracks

#### **+ Button (Add Files)**
- **Location**: Top-left of playlist
- **Click**: Opens file browser
- **Multi-Select**: Hold Ctrl (Windows) or Cmd (Mac) to select multiple files
- **Shift-Select**: Click first file, hold Shift, click last file to select range
- **Processing**: Automatically analyzes BPM for each track

#### **Drag & Drop**
- **Source**: Windows Explorer, Finder, or file manager
- **Target**: Drop files onto playlist area
- **Visual Feedback**: Blue highlight overlay with "Drop audio files here" text
- **Supports**: Single or multiple files simultaneously
- **Processing**: Same as + button (BPM analysis)


### Playlist Display Columns

#### **#** (Number)
- **Function**: Track position in playlist (001, 002, etc.)
- **Auto-Update**: Renumbers when tracks removed

#### **Title**
- **Source**: File name (without extension)
- **Width**: 348 pixels
- **Usage**: Track identification

#### **Artist**
- **Default**: "Unknown"
- **Source**: File metadata (if available)
- **Width**: 112 pixels

#### **Time**
- **Format**: MM:SS or HH:MM:SS
- **Source**: Track duration from audio file
- **Width**: 90 pixels

#### **BPM**
- **Source**: Automatic detection algorithm
- **Display**: Integer value (e.g., 128)
- **Accuracy**: ±2 BPM typical
- **Width**: 64 pixels


### Playlist Row Actions

#### **Left Deck Button** (↑ Icon)
- **Click**: Load track into left deck
- **Behavior**: 
  - Loads audio file
  - Analyzes BPM
  - Loads waveform
  - Restores hot cues from hotcues.json
  - Restores mixer settings from mixer.json
- **Visual**: Button in each row

#### **Right Deck Button** (↑ Icon)
- **Click**: Load track into right deck
- **Behavior**: Same as left deck button
- **Location**: Next to left deck button

#### **Remove Button** (Trash Icon)
- **Click**: Remove track from playlist
- **Behavior**: Immediate removal, re-numbers remaining tracks
- **Persistence**: Updates playlist.json


### Playlist Interaction

#### **Mouse Hover**
- **Visual**: Row background changes to highlight color (gray)
- **Usage**: Indicates current row

#### **Scrolling**
- **Mouse Wheel**: Scroll through long playlists
- **Viewport**: Scrollable container for unlimited tracks


================================================================================

## PERSISTENCE FEATURES (Auto-Save)

### Playlist Persistence
- **File**: `Documents/DJ App/playlist.json`
- **Saved**: Every time track added/removed
- **Loaded**: Automatically on application startup
- **Data**: Track paths, title, artist, duration, BPM

### Hot Cue Persistence (Per-Track)
- **File**: `Documents/DJ App/hotcues.json`
- **Trigger**: Every cue add/remove/clear
- **Key**: Full file path
- **Data**: Array of 8 cue times (seconds) for each track
- **Behavior**: 
  - When you set cues on a track, they're saved
  - When you reload that track, cues restore automatically
  - Different tracks have different cues

### Mixer State Persistence (Per-Track)
- **File**: `Documents/DJ App/mixer.json`
- **Trigger**: Every mixer control change (knob/fader)
- **Key**: Full file path
- **Data**: 
  - Trim value
  - High EQ value
  - Mid EQ value
  - Low EQ value
  - Volume fader value
- **Behavior**:
  - EQ adjustments saved per track
  - When track reloaded, mixer settings restore
  - Enables pre-prepared EQ setups


================================================================================

## KEYBOARD SHORTCUTS

### Global Controls
- **Spacebar**: Play/Pause focused deck (if implemented)
- **Shift + [Action]**: Modifier key for alternate behaviors

### Hot Cue Shortcuts
- **Shift + Pad Click**: Clear individual hot cue
- **Shift + CUE button**: Alternative cue behavior (if implemented)

### Deck Selection
- **Tab**: Switch focus between decks (if implemented)


================================================================================

## WORKFLOW EXAMPLES

### Basic Mixing Workflow
1. **Load Tracks**: 
   - Click + button and select multiple files, or drag & drop
   - Tracks appear in playlist with BPM analyzed

2. **Load First Track**: 
   - Click left deck button (↑) on a track
   - Track loads into left deck with waveform

3. **Set Hot Cues** (Optional):
   - Play track and find key points (intro, drop, breakdown)
   - Click empty hot cue pad at desired moments
   - Cues save automatically per track

4. **Adjust EQ** (Optional):
   - Set track-specific EQ using HIGH, MID, LOW knobs
   - Settings save automatically for this track

5. **Start Playback**:
   - Click PLAY button on left deck
   - Music starts playing

6. **Load Second Track**:
   - Click right deck button (↑) on next track
   - Track loads and restores any saved cues/EQ

7. **Cue Second Track**:
   - Click CUE button on right deck to set cue point
   - Hold CUE to preview and find beat alignment

8. **Beatmatch**:
   - Use SYNC button for automatic match, OR
   - Use TEMPO slider and jog wheel for manual adjustment
   - Watch waveforms and beat grids for visual alignment

9. **Mix Tracks**:
   - Adjust EQ (cut lows on old track, boost on new)
   - Use CROSSFADER to transition between tracks
   - Use VOLUME faders for additional control

10. **Use Hot Cues**:
    - Click cue pads to jump to prepared points
    - Create mashups by jumping between tracks

### Advanced: Loop Extension
1. Find breakdown or instrumental section
2. Click **4** or **8** beat loop button during playback
3. Loop plays seamlessly for extended time
4. Use EQ to filter sound while looping
5. Click loop button again to exit and continue playback

### Advanced: Sample Triggering
1. Switch to **SAMPLER mode**
2. Click empty pad to load effect samples (air horn, vocal clips)
3. During mix, click pads to trigger effects
4. Layer multiple samples simultaneously


================================================================================

## TROUBLESHOOTING

### Audio Issues
- **No Sound**: Check audio device settings in system
- **Crackling**: Increase buffer size in audio settings
- **Latency**: Use ASIO driver on Windows

### File Issues
- **Track Won't Load**: Verify file format is supported
- **BPM Incorrect**: Detection may fail on ambient/experimental music
- **Cues Not Saving**: Check write permissions in Documents folder

### Performance Issues
- **Lag/Stuttering**: Close other applications
- **High CPU**: Reduce number of active effects/samples
- **Waveform Slow**: Reduce zoom level or simplify view


================================================================================

## TIPS FOR BEST RESULTS

### BPM Detection
- Works best with 4/4 time signatures
- Electronic, hip-hop, house music: Very accurate
- Acoustic, live recordings: May need manual adjustment
- Use TEMPO slider if BPM detection is off

### Hot Cue Strategy
- Pad 1: Track intro
- Pad 2: First drop/chorus
- Pad 3: Breakdown
- Pad 4: Second drop
- Pad 5-8: Vocal hooks, effects, special moments

### EQ Mixing Technique
1. **Bass Swap**: Cut LOW on outgoing track while boosting on incoming
2. **Filter Transition**: Cut all EQ on one track, gradually restore on other
3. **Frequency Space**: Cut MID/HIGH on one track to make room for other

### Performance Preparation
1. Load entire setlist into playlist
2. Analyze all BPMs (automatic)
3. Set hot cues on all tracks (pads 1-4 minimum)
4. Pre-adjust EQ if tracks need fixing (bright cymbals, muddy bass)
5. Save everything automatically
6. Reload and perform with all prep work intact


================================================================================

## ADVANCED FEATURES SUMMARY

- **Dual Independent Decks**: Full-featured playback on both sides
- **Real-Time BPM Detection**: Automatic tempo analysis on load
- **Beat Grid Visualization**: Visual beatmatching aids
- **8 Hot Cues per Deck**: Instant jump points with persistence
- **8 Sampler Pads per Deck**: One-shot effect triggering
- **3-Band EQ + Trim**: Professional frequency mixing
- **Beat-Quantized Loops**: Musical loop lengths (1/4 to 8 beats)
- **Per-Track State Memory**: Cues and mixer settings saved per track
- **Multi-Format Support**: MP3, WAV, FLAC, M4A, AIF, AIFF, OGG
- **Multi-Select File Import**: Batch library building
- **Drag-and-Drop**: Fast track addition
- **Professional Crossfader**: Equal-power mixing