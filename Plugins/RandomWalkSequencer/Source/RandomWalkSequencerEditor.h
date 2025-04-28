#pragma once

#include <JuceHeader.h>
#include "RandomWalkSequencer.h"

/**
 * Editor component for the RandomWalkSequencer plugin
 * Provides the user interface for controlling the sequencer parameters
 */
class RandomWalkSequencerEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    /**
     * Constructor - initializes the UI components
     * @param p Reference to the RandomWalkSequencer processor
     */
    RandomWalkSequencerEditor(RandomWalkSequencer& p);

    /**
     * Destructor - cleans up resources and stops timer
     */
    ~RandomWalkSequencerEditor() override;

    // Component methods
    /**
     * Paints the editor background and header text
     */
    void paint(juce::Graphics&) override;

    /**
     * Positions all UI components within the editor
     */
    void resized() override;

    /**
     * Timer callback to update UI state from the processor
     * Refreshes controls and display at regular intervals
     */
    void timerCallback() override;

    /**
     * Updates density slider enabled state based on manual step mode
     * Disables density control when in manual step mode
     */
    void updateDensitySliderState();

    /**
     * Updates the manual step toggle button state
     * @param state New state for the toggle button
     */
    void updateManualStepToggle(bool state);

private:
    // Reference to the processor
    RandomWalkSequencer& randomWalkProcessor; // Renamed from 'processor' to avoid shadowing

    //==============================================================================
    // UI Components

    /**
     * Dropdown menu for selecting the step rate
     */
    juce::ComboBox rateComboBox;

    /**
     * Slider for adjusting the number of active steps
     */
    juce::Slider densitySlider;

    /**
     * Slider for adjusting the sequence start offset
     */
    juce::Slider offsetSlider;

    /**
     * Slider for adjusting the note duration (gate time)
     */
    juce::Slider gateSlider;

    /**
     * Slider for adjusting the root note
     */
    juce::Slider rootSlider;

    /**
     * Button for generating a new random sequence
     */
    juce::TextButton randomizeButton;

    /**
     * Button for toggling playback
     */
    juce::TextButton playButton;

    /**
     * Dropdown menu for selecting the pattern type
     */
    juce::ComboBox patternTypeComboBox;

    /**
     * Label for the pattern type dropdown
     */
    juce::Label patternTypeLabel;

    /**
     * Toggle button for syncing to host transport
     */
    juce::ToggleButton syncButton;

    /**
     * Toggle button for manual step mode
     * Enables individual steps to be toggled on/off
     */
    juce::ToggleButton manualStepToggle;

    /**
     * Label for the manual step toggle
     */
    juce::Label manualStepLabel;

    /**
     * Button for transposing up one octave
     */
    juce::TextButton transposeUpButton;

    /**
     * Button for transposing down one octave
     */
    juce::TextButton transposeDownButton;

    /**
     * Label for transpose buttons
     */
    juce::Label transposeLabel;

    /**
     * Button for switching to mono mode (all steps play root note)
     */
    juce::TextButton monoButton;

    //==============================================================================
    /**
     * Step display component that visualizes the sequence pattern
     * Allows interactive editing of step values by dragging
     */
    class StepDisplay : public juce::Component
    {
    public:
        /**
         * Constructor for the step display
         * @param proc Reference to the RandomWalkSequencer processor
         * @param ed Reference to the editor component
         */
        StepDisplay(RandomWalkSequencer& proc, RandomWalkSequencerEditor& ed);

        /**
         * Draws the step sequence visualization
         * Shows current step, note values, and enabled/disabled states
         */
        void paint(juce::Graphics& g) override;

        // Mouse interaction methods
        /**
         * Handles mouse button press on a step
         * Starts step value editing
         */
        void mouseDown(const juce::MouseEvent& e) override;

        /**
         * Handles mouse drag to adjust step values
         * Updates the step value based on vertical position
         */
        void mouseDrag(const juce::MouseEvent& e) override;

        /**
         * Handles mouse button release
         * Finalizes step value editing
         */
        void mouseUp(const juce::MouseEvent& e) override;

        /**
         * Handles double-click to toggle step enabled/disabled
         * Activates manual step mode if not already active
         */
        void mouseDoubleClick(const juce::MouseEvent& e) override;

    private:
        RandomWalkSequencer& processor;
        RandomWalkSequencerEditor& editor;
        int draggedStep = -1;  // Currently dragged step

        /**
         * Converts vertical position to note value
         * @param y Vertical position in pixels
         * @return Note value offset (-12 to +12)
         */
        int yPositionToNoteValue(float y);

        /**
         * Determines which step was clicked based on mouse position
         * @param e Mouse event containing position information
         * @return Step index (0-15)
         */
        int getStepNumberFromMousePosition(const juce::MouseEvent& e);
    };

    /**
     * Step display component instance
     */
    StepDisplay stepDisplay;

    //==============================================================================
    // UI Labels

    juce::Label rateLabel;
    juce::Label densityLabel;
    juce::Label offsetLabel;
    juce::Label gateLabel;
    juce::Label rootLabel;

    /**
     * Slider for adjusting internal BPM when not synced to host
     */
    juce::Slider bpmSlider;

    /**
     * Label for the BPM slider
     */
    juce::Label bpmLabel;

    /**
     * Updates the root note display text to show note name
     * Converts MIDI note number to note name with octave
     */
    void updateRootNoteDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomWalkSequencerEditor)
};