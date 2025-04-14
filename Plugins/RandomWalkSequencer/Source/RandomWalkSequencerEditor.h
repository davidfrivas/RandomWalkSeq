#pragma once

#include <JuceHeader.h>
#include "RandomWalkSequencer.h"

class RandomWalkSequencerEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    RandomWalkSequencerEditor(RandomWalkSequencer& p);
    ~RandomWalkSequencerEditor() override;

    // Component methods
    void paint(juce::Graphics&) override;
    void resized() override;

    // Timer callback to update UI
    void timerCallback() override;

private:
    // Reference to the processor - renamed to avoid shadowing
    RandomWalkSequencer& randomWalkProcessor; // Renamed from 'processor' to avoid shadowing

    // UI Components
    juce::ComboBox rateComboBox;
    juce::Slider densitySlider;
    juce::Slider offsetSlider;
    juce::Slider gateSlider;
    juce::Slider rootSlider;
    juce::TextButton randomizeButton;
    juce::TextButton playButton;
    juce::ComboBox patternTypeComboBox;  // Pattern type selector
    juce::Label patternTypeLabel;  // Label for pattern type
    juce::ToggleButton syncButton;  // Transport sync toggle

    // Step display
    class StepDisplay : public juce::Component
    {
    public:
        StepDisplay(RandomWalkSequencer& proc);

        void paint(juce::Graphics& g) override;

        // Add mouse interaction methods
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;

    private:
        RandomWalkSequencer& processor;
        int draggedStep = -1;  // Currently dragged step

        // Helper to convert y position to note value
        int yPositionToNoteValue(float y);
        // Helper to determine which step was clicked
        int getStepNumberFromMousePosition(const juce::MouseEvent& e);
    };

    StepDisplay stepDisplay;

    // Labels
    juce::Label rateLabel;
    juce::Label densityLabel;
    juce::Label offsetLabel;
    juce::Label gateLabel;
    juce::Label rootLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomWalkSequencerEditor)
};