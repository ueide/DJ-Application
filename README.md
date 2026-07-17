# Dual-Deck DJ Application
A high-performance, cross-platform desktop audio mixing application built from scratch. It is designed with low-latency audio processing and custom vector graphics rendering.

## Description
A desktop DJ application enabling real-time audio manipulation and mixing across dual decks. This project is built with Modern C++ and the JUCE framework, bypassing the default operating system components to render a fully custom user interface. To handle demanding audio calculations without freezing the display, the application employs a multi-threaded architecture that completely separates audio processing from UI rendering.

![Live](https://github.com/ueide/DJ-Application/blob/main/assets/DJ_App_gif.gif)

## Features
- **Multi-Threaded Waveform Generation:** The application utilises an asynchronous background thread (`AudioThumbnailCache`) to analyse heavy audio files. This prevents the main UI thread from lagging or dropping frames while rendering interactive waveforms during intensive audio playback.

- **Custom UI Component Rendering:** Manual implementation of `paint()` and `resized()` lifecycles using vector graphics (SVGs). This ensures a fluid, hardware-accelerated 60 FPS user experience that scales perfectly across different screen sizes without framework overhead.

- **Real-Time DSP Engine:** Features a custom-mapped 3-band EQ frequency isolation filter and a smooth logarithmic crossfader. The mathematical curves guarantee transparent, click-free audio transitions during live mixing.

- **State and Playlist Persistence:** Implementation of automatic data serialization using `juce::JSON`. User playlist data, active track selections, and custom mixer layouts are verified and saved locally, ensuring seamless session recovery upon app restart.

- **Decoupled Architecture:** Component communication relies entirely on modern C++ lambda callbacks (such as `onLeftEQChanged`). This pattern eliminates tight dependencies between the interface controls and the underlying audio engine, making the codebase highly scalable and easy to test.

## Live Demo
You can [access the live presentation page here](https://ueide.github.io/DJ-Application/)

## Technology Stack
- **Core Language:** Modern C++ (C++17 / C++20)

- **Framework:** JUCE SDK (Audio application lifecycle and low-level system hooks)

- **Data Format:** JSON (For state management and data persistence)
