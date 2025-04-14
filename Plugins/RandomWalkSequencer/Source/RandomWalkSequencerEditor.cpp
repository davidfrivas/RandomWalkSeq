#include "RandomWalkSequencerEditor.h"
#include <iostream>

#define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl

// Modified constructor with direct parameter control
RandomWalkSequencerEditor::RandomWalkSequencerEditor(RandomWalkSequencer& p)
    : AudioProcessorEditor(&p)
    , randomWalkProcessor(p) // Renamed from 'processor' to avoid shadowing
    , stepDisplay(p)
{
    DEBUG_LOG("Editor constructor start");

    // Rate label
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rateLabel);

    // Rate combo box setup
    rateComboBox.addItemList(juce::StringArray("1/32", "1/16", "1/8", "1/4", "1/3", "1/2", "1", "2", "3", "4"), 1);
    rateComboBox.setSelectedItemIndex(randomWalkProcessor.getRate()); // Using renamed processor
    rateComboBox.setJustificationType(juce::Justification::centred);
    rateComboBox.onChange = [this] { randomWalkProcessor.setRate(rateComboBox.getSelectedItemIndex()); }; // Using renamed processor
    addAndMakeVisible(rateComboBox);

    // Density slider
    densityLabel.setText("Density", juce::dontSendNotification);
    densityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(densityLabel);

    densitySlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    densitySlider.setRange(1, 16, 1);
    densitySlider.setValue(randomWalkProcessor.getDensity()); // Using renamed processor
    densitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    densitySlider.onValueChange = [this] { randomWalkProcessor.setDensity(static_cast<int>(densitySlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(densitySlider);

    // Offset slider
    offsetLabel.setText("Offset", juce::dontSendNotification);
    offsetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(offsetLabel);

    offsetSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    offsetSlider.setRange(0, 15, 1);
    offsetSlider.setValue(randomWalkProcessor.getOffset()); // Using renamed processor
    offsetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    offsetSlider.onValueChange = [this] { randomWalkProcessor.setOffset(static_cast<int>(offsetSlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(offsetSlider);

    // Gate slider
    gateLabel.setText("Gate", juce::dontSendNotification);
    gateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gateLabel);

    gateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gateSlider.setRange(0.1, 1.0, 0.01);
    gateSlider.setValue(randomWalkProcessor.getGate()); // Using renamed processor
    gateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    gateSlider.onValueChange = [this] { randomWalkProcessor.setGate(static_cast<float>(gateSlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(gateSlider);

    // Root slider
    rootLabel.setText("Root", juce::dontSendNotification);
    rootLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rootLabel);

    rootSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    rootSlider.setRange(60, 72, 1); // MIDI notes from C4 to C5
    rootSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Wider for note name display
    // Update the note name display function that's called when the slider value changes
    rootSlider.onValueChange = [this] {
        int value = static_cast<int>(rootSlider.getValue());
        randomWalkProcessor.setRoot(value);

        // Update note name display
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int noteIndex = value % 12;
        int octave = value / 12 - 1;  // MIDI note 60 is C4
        juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
        rootSlider.setTextValueSuffix(" (" + noteName + ")");
    };
    addAndMakeVisible(rootSlider);

    // Initialize note name display
    rootSlider.onValueChange();

    // Randomize button
    randomizeButton.setButtonText("Randomize");
    randomizeButton.onClick = [this] { randomWalkProcessor.randomizeSequence(); }; // Using renamed processor
    addAndMakeVisible(randomizeButton);

    // Play button
    playButton.setButtonText("Play");
    playButton.setClickingTogglesState(true);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    playButton.onClick = [this] {
        bool isPlaying = playButton.getToggleState();
        randomWalkProcessor.setPlaying(isPlaying);
        playButton.setButtonText(isPlaying ? "Stop" : "Play");
    };
    addAndMakeVisible(playButton);

    // Pattern type selector
    patternTypeLabel.setText("Pattern", juce::dontSendNotification);
    patternTypeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(patternTypeLabel);

    patternTypeComboBox.addItemList(juce::StringArray(
        "Random Walk", "Ascending", "Descending", "Arpeggio"), 1);
    patternTypeComboBox.setSelectedItemIndex(0);
    patternTypeComboBox.onChange = [this] {
        // Generate a new sequence with the selected pattern type
        randomWalkProcessor.randomizeSequence(patternTypeComboBox.getSelectedItemIndex());
    };
    addAndMakeVisible(patternTypeComboBox);

    // Transport sync toggle
    syncButton.setButtonText("Sync to Host Transport");
    syncButton.setToggleState(true, juce::dontSendNotification);
    syncButton.onClick = [this] {
        randomWalkProcessor.setSyncToHostTransport(syncButton.getToggleState());
    };
    addAndMakeVisible(syncButton);

    // Step display
    addAndMakeVisible(stepDisplay);
    stepDisplay.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);

    // Set up timer to refresh UI
    startTimerHz(30);

    // Set initial size
    setSize(600, 400);

    DEBUG_LOG("Editor constructor end");
}

RandomWalkSequencerEditor::~RandomWalkSequencerEditor()
{
    stopTimer();
}

void RandomWalkSequencerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawText("Random Walk Sequencer", getLocalBounds(), juce::Justification::centredTop, true);
}

void RandomWalkSequencerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Calculate the total height needed
    int totalHeight = 40 + 150 + 30 + 10 + (40 + 10) * 5; // Header + Display + Sync + Spacing + 5 controls with spacing

    // Set a minimum size for the editor to ensure all controls are visible
    setSize(juce::jmax(600, getWidth()), juce::jmax(totalHeight, getHeight()));

    // Now continue with layout
    area = getLocalBounds().reduced(10);

    // Header section
    auto headerArea = area.removeFromTop(40);

    // Add pattern selector to the left
    auto patternArea = headerArea.removeFromLeft(200);
    patternTypeLabel.setBounds(patternArea.removeFromLeft(80));
    patternTypeComboBox.setBounds(patternArea);

    // Add buttons to the right
    auto buttonArea = headerArea.removeFromRight(240);
    randomizeButton.setBounds(buttonArea.removeFromLeft(120));
    playButton.setBounds(buttonArea);

    // Step display
    auto displayArea = area.removeFromTop(150);
    stepDisplay.setBounds(displayArea);

    // Transport sync toggle below the step display
    syncButton.setBounds(area.removeFromTop(30));

    area.removeFromTop(10); // Add spacing

    // Controls section - create rows for the parameters with consistent heights
    auto controlHeight = 40;

    // Rate
    auto rateArea = area.removeFromTop(controlHeight);
    rateLabel.setBounds(rateArea.removeFromLeft(80));
    rateComboBox.setBounds(rateArea);

    area.removeFromTop(10); // Spacing

    // Density
    auto densityArea = area.removeFromTop(controlHeight);
    densityLabel.setBounds(densityArea.removeFromLeft(80));
    densitySlider.setBounds(densityArea);

    area.removeFromTop(10); // Spacing

    // Offset
    auto offsetArea = area.removeFromTop(controlHeight);
    offsetLabel.setBounds(offsetArea.removeFromLeft(80));
    offsetSlider.setBounds(offsetArea);

    area.removeFromTop(10); // Spacing

    // Gate - Make sure there's room for this
    auto gateArea = area.removeFromTop(controlHeight);
    gateLabel.setBounds(gateArea.removeFromLeft(80));
    gateSlider.setBounds(gateArea.withWidth(juce::jmax(50, gateArea.getWidth())));

    area.removeFromTop(10); // Spacing

    // Root - Make sure there's room for this
    auto rootArea = area.removeFromTop(controlHeight);
    rootLabel.setBounds(rootArea.removeFromLeft(80));
    rootSlider.setBounds(rootArea.withWidth(juce::jmax(50, rootArea.getWidth())));

    // Debug print to see if we have enough space
    DEBUG_LOG("Remaining area height: " << area.getHeight());
}

void RandomWalkSequencerEditor::timerCallback()
{
    // Update controls from processor values, if needed
    if (rateComboBox.getSelectedItemIndex() != randomWalkProcessor.getRate()) // Using renamed processor
        rateComboBox.setSelectedItemIndex(randomWalkProcessor.getRate()); // Using renamed processor

    if (static_cast<int>(densitySlider.getValue()) != randomWalkProcessor.getDensity()) // Using renamed processor
        densitySlider.setValue(randomWalkProcessor.getDensity()); // Using renamed processor

    if (static_cast<int>(offsetSlider.getValue()) != randomWalkProcessor.getOffset()) // Using renamed processor
        offsetSlider.setValue(randomWalkProcessor.getOffset()); // Using renamed processor

    if (std::abs(gateSlider.getValue() - randomWalkProcessor.getGate()) > 0.01) // Using renamed processor
        gateSlider.setValue(randomWalkProcessor.getGate()); // Using renamed processor

    if (static_cast<int>(rootSlider.getValue()) != randomWalkProcessor.getRoot()) // Using renamed processor
        rootSlider.setValue(randomWalkProcessor.getRoot()); // Using renamed processor

    // Update play button state
    bool isProcessorPlaying = randomWalkProcessor.getIsPlaying();
    if (playButton.getToggleState() != isProcessorPlaying)
    {
        playButton.setToggleState(isProcessorPlaying, juce::dontSendNotification);
        playButton.setButtonText(isProcessorPlaying ? "Stop" : "Play");
    }

    // Repaint the step display
    stepDisplay.repaint();
}

RandomWalkSequencerEditor::StepDisplay::StepDisplay(RandomWalkSequencer& proc)
    : processor(proc)
{
    // Enable mouse events only
    setInterceptsMouseClicks(true, true);

    // Remove the setTooltip line

    // Make cursor change to indicate editable area
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

int RandomWalkSequencerEditor::StepDisplay::getStepNumberFromMousePosition(const juce::MouseEvent& e)
{
    const int numSteps = 16;
    const float w = (float)getWidth() / numSteps;

    // Calculate which step was clicked
    int stepNumber = (int)(e.position.x / w);

    // Ensure valid range
    return juce::jlimit(0, numSteps - 1, stepNumber);
}

int RandomWalkSequencerEditor::StepDisplay::yPositionToNoteValue(float y)
{
    float h = (float)getHeight();
    float midPoint = h * 0.5f;

    // Convert y position to note value
    // The range is -12 to +12 semitones
    float relativeY = midPoint - y;
    int noteValue = (int)(relativeY / (h / 24.0f));

    // Limit to reasonable range
    return juce::jlimit(-12, 12, noteValue);
}

void RandomWalkSequencerEditor::StepDisplay::mouseDown(const juce::MouseEvent& e)
{
    // Identify which step was clicked
    draggedStep = getStepNumberFromMousePosition(e);
}

void RandomWalkSequencerEditor::StepDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedStep >= 0)
    {
        // Convert mouse y position to note value
        int noteValue = yPositionToNoteValue(e.position.y);

        // Update the sequence step value
        processor.setSequenceValue(draggedStep, noteValue);

        // Redraw the component
        repaint();
    }
}

void RandomWalkSequencerEditor::StepDisplay::mouseUp(const juce::MouseEvent& /*e*/)
{
    // Reset dragged step
    draggedStep = -1;
}

// Implement StepDisplay
void RandomWalkSequencerEditor::StepDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    const int numSteps = 16;
    const float w = (float)getWidth() / numSteps;
    const float h = (float)getHeight();
    const float midPoint = h * 0.5f;

    try {
        // Get current parameters from processor
        int currentDensity = processor.getDensity();
        int currentOffset = processor.getOffset();

        // Get the current step (un-offset)
        int baseCurrentStep = processor.getCurrentStep();

        // Calculate the actual playing step with offset
        int actualCurrentStep = (baseCurrentStep + currentOffset) % numSteps;

        // Draw steps
        for (int i = 0; i < numSteps; ++i)
        {
            // Determine if this step is part of the loop range
            bool isInLoopRange = false;

            // Check if this step is within the current playing range
            for (int step = 0; step < currentDensity; step++) {
                int loopStep = (currentOffset + step) % numSteps;
                if (i == loopStep) {
                    isInLoopRange = true;
                    break;
                }
            }

            // Determine if this is the current playing step
            bool isCurrent = (i == actualCurrentStep);
            bool isBeingDragged = (i == draggedStep);

            // Draw step rectangle
            juce::Rectangle<float> stepRect(i * w, 0, w - 2, h);

            // Color based on step status
            if (isBeingDragged)
                g.setColour(juce::Colours::brown);  // Change from yellow to brown for dragged steps
            else if (isCurrent)
                g.setColour(juce::Colours::orange);  // Current playing step
            else if (isInLoopRange)
                g.setColour(juce::Colours::lightgreen);  // In play range but not current
            else
                g.setColour(juce::Colours::grey);  // Outside play range

            g.fillRect(stepRect);

            // Draw note value as a line
            int noteOffset = processor.getSequenceValue(i);
            float lineY = midPoint - (noteOffset * (h / 24.0f)); // Scale to fit in view

            // Draw the note line with a different color when dragging
            g.setColour(isBeingDragged ? juce::Colours::white : juce::Colours::white);  // Always use white for the line
            g.drawLine(i * w, lineY, (i + 1) * w - 2, lineY, isBeingDragged ? 3.0f : 2.0f);

            // Draw note value text
            g.setFont(12.0f);
            g.setColour(juce::Colours::white);  // Always use white for text to ensure readability
            g.drawText(juce::String(noteOffset),
                      stepRect.reduced(2),
                      juce::Justification::topLeft,
                      true);

            // Draw step number for clarity
            g.setFont(10.0f);
            g.drawText(juce::String(i),
                     stepRect.reduced(2),
                     juce::Justification::bottomRight,
                     true);
        }

        // Draw center line (for reference)
        g.setColour(juce::Colours::darkgrey.brighter(0.3f));
        g.drawLine(0, midPoint, getWidth(), midPoint, 1.0f);
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Exception in paint: " << e.what());
    }
    catch (...) {
        DEBUG_LOG("Unknown exception in paint");
    }
}