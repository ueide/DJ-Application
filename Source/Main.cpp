/*
    Main.cpp
    Application entry point and main window setup.
    Main responsibilities:
    - Define the main application class (DJ Application) that inherits from juce::JUCEApplication.
    - Implement application lifecycle methods: initialise, shutdown, systemRequestedQuit, anotherInstanceStarted.
    - Define the MainWindow class that inherits from juce::DocumentWindow, responsible for creating and managing the main application window.
    - Set the initial size and position of the main window, and handle the close button event to quit the application.
*/


#include <JuceHeader.h>
#include "MainComponent.h"


//--- Application class ---//
class DJAppApplication  : public juce::JUCEApplication {

    public:
        DJAppApplication() {}

        const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
        const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
        bool moreThanOneInstanceAllowed() override             { return true; }

        // Initialize the application
        void initialise (const juce::String& commandLine) override {
            mainWindow.reset (new MainWindow (getApplicationName()));
        }

        // Shutdown the application
        void shutdown() override{
            mainWindow = nullptr; // (deletes window)
        }

        // Handle system request to quit the application
        void systemRequestedQuit() override {
            quit();
        }

        // Handle another instance of the application being started
        void anotherInstanceStarted (const juce::String& commandLine) override {

        }

        //--- Main window class ---//
        class MainWindow    : public juce::DocumentWindow {
            public:
                MainWindow (juce::String name) : DocumentWindow (name,
                                    juce::Desktop::getInstance().getDefaultLookAndFeel()
                                        .findColour (juce::ResizableWindow::backgroundColourId),
                                    DocumentWindow::allButtons) {
                    setUsingNativeTitleBar (true);
                    setContentOwned (new MainComponent(), true);

                    // Set the initial size and position of the window
                    #if JUCE_IOS || JUCE_ANDROID
                        setFullScreen (true);
                    #else
                        setResizable (true, true);
                        centreWithSize (1280, 780); // Initial size
                    #endif

                    setVisible (true);
                }

                // Handle the close button press event
                void closeButtonPressed() override {
                    JUCEApplication::getInstance()->systemRequestedQuit();
                }

            private:
                // The Juce macro that helps detect memory leaks
                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
        };

    private:
        std::unique_ptr<MainWindow> mainWindow;
};
// This macro generates the main() routine that launches the app
START_JUCE_APPLICATION (DJAppApplication);