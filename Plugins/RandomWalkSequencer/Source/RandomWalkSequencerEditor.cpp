#include <memory>
#include <iostream>
#define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl;

#include "RandomWalkSequencer.h"
#include "RandomWalkSequencerEditor.h"

/**
 * Constructor - initializes all UI components and connects them to the processor
 * @param p Reference to the RandomWalkSequencer processor
 */
RandomWalkSequencerEditor::RandomWalkSequencerEditor(RandomWalkSequencer& p)
    : AudioProcessorEditor(&p)
    , randomWalkProcessor(p)
    , stepDisplay(p, *this)
{
    DEBUG_LOG("Editor constructor start");

    // Rate label and combo box setup
    rateLabel.setText("Rate", juce::dontSendNotification);
    rateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rateLabel);

    // Rate combo box setup - musical note values
    rateComboBox.addItemList(juce::StringArray("1/32", "1/16", "1/8", "1/4", "1/3", "1/2", "1", "2", "3", "4"), 1);
    rateComboBox.setSelectedItemIndex(randomWalkProcessor.getRate()); // Using renamed processor
    rateComboBox.setJustificationType(juce::Justification::centred);
    rateComboBox.onChange = [this] { randomWalkProcessor.setRate(rateComboBox.getSelectedItemIndex()); }; // Using renamed processor
    addAndMakeVisible(rateComboBox);

    // Density slider - controls number of active steps
    densityLabel.setText("Density", juce::dontSendNotification);
    densityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(densityLabel);

    densitySlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    densitySlider.setRange(1, 16, 1);
    densitySlider.setValue(randomWalkProcessor.getDensity()); // Using renamed processor
    densitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    densitySlider.onValueChange = [this] { randomWalkProcessor.setDensity(static_cast<int>(densitySlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(densitySlider);

    // Offset slider - controls sequence start position
    offsetLabel.setText("Offset", juce::dontSendNotification);
    offsetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(offsetLabel);

    offsetSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    offsetSlider.setRange(0, 15, 1);
    offsetSlider.setValue(randomWalkProcessor.getOffset()); // Using renamed processor
    offsetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    offsetSlider.onValueChange = [this] { randomWalkProcessor.setOffset(static_cast<int>(offsetSlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(offsetSlider);

    // Gate slider - controls note duration
    gateLabel.setText("Gate", juce::dontSendNotification);
    gateLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gateLabel);

    gateSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    gateSlider.setRange(0.1, 1.0, 0.01);
    gateSlider.setValue(randomWalkProcessor.getGate()); // Using renamed processor
    gateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    gateSlider.onValueChange = [this] { randomWalkProcessor.setGate(static_cast<float>(gateSlider.getValue())); }; // Using renamed processor
    addAndMakeVisible(gateSlider);

    // Root slider - controls base MIDI note
    rootLabel.setText("Root", juce::dontSendNotification);
    rootLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rootLabel);

    rootSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    rootSlider.setRange(12, 120, 1); // From C0 to C9
    rootSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20); // Wider for note name display
    // Update the note name display function that's called when the slider value changes
    rootSlider.onValueChange = [this] {
        int value = static_cast<int>(rootSlider.getValue());
        randomWalkProcessor.setRoot(value);
        updateRootNoteDisplay();
    };
    addAndMakeVisible(rootSlider);

    // Initialize slider with processor's value and update display
    rootSlider.setValue(randomWalkProcessor.getRoot(), juce::dontSendNotification);
    updateRootNoteDisplay();

    // Transpose Octave controls
    transposeLabel.setText("Transpose Octave", juce::dontSendNotification);
    transposeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(transposeLabel);

    // Down button - transposes down one octave
    transposeDownButton.setButtonText("v");
    transposeDownButton.onClick = [this] {
        randomWalkProcessor.transposeOctaveDown();
        rootSlider.setValue(randomWalkProcessor.getRoot(), juce::dontSendNotification);
        updateRootNoteDisplay();
    };
    addAndMakeVisible(transposeDownButton);

    // Up button - transposes up one octave
    transposeUpButton.setButtonText("^");
    transposeUpButton.onClick = [this] {
        randomWalkProcessor.transposeOctaveUp();
        rootSlider.setValue(randomWalkProcessor.getRoot(), juce::dontSendNotification);
        updateRootNoteDisplay();
    };
    addAndMakeVisible(transposeUpButton);

    // Randomize button - generates new pattern
    randomizeButton.setButtonText("Randomize");
    randomizeButton.onClick = [this] { randomWalkProcessor.randomizeSequence(patternTypeComboBox.getSelectedItemIndex()); }; // Using renamed processor
    addAndMakeVisible(randomizeButton);

    // Mono button - sets all steps to play root note
    monoButton.setButtonText("Mono");
    monoButton.onClick = [this] {
        randomWalkProcessor.setMonoMode();
    };
    addAndMakeVisible(monoButton);

    // Play button - controls playback when not synced to host
    playButton.setButtonText("Play");
    playButton.setClickingTogglesState(true);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    playButton.onClick = [this] {
        // Only allow manual control when not synced to host
        if (!randomWalkProcessor.getSyncToHostTransport())
        {
            // We're not synced, so use the play button to control playback
            bool isPlaying = playButton.getToggleState();

            // Update the processor's playing state
            randomWalkProcessor.setPlaying(isPlaying);

            // Update the button text immediately for better responsiveness
            playButton.setButtonText(isPlaying ? "Stop" : "Play");
        }
        else
        {
            // When synced, explain to user
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Transport Sync Active",
                "The sequencer is synced to the host transport.\n"
                "Use Ableton's play controls instead.",
                "OK");

            // Reset toggle state to match processor
            playButton.setToggleState(randomWalkProcessor.getIsPlaying(), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(playButton);

    // Pattern type selector
    patternTypeLabel.setText("Pattern", juce::dontSendNotification);
    patternTypeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(patternTypeLabel);

    // Pattern type selector - different sequence patterns
    patternTypeComboBox.addItemList(juce::StringArray(
        "Random Walk", "Ascending", "Descending", "Arpeggio"), 1);
    patternTypeComboBox.setSelectedItemIndex(0, juce::dontSendNotification); // Use dontSendNotification!
    patternTypeComboBox.onChange = [this] {
        // Generate a new sequence with the selected pattern type
        randomWalkProcessor.randomizeSequence(patternTypeComboBox.getSelectedItemIndex());
    };
    addAndMakeVisible(patternTypeComboBox);

    // Transport sync toggle - syncs to host transport
    syncButton.setButtonText("Sync to Host Transport");
    syncButton.setToggleState(false, juce::dontSendNotification);
    syncButton.onClick = [this] {
        bool syncState = syncButton.getToggleState();
        randomWalkProcessor.setSyncToHostTransport(syncState);
        bpmSlider.setEnabled(!syncState);
        bpmLabel.setAlpha(syncState ? 0.5f : 1.0f);
    };
    addAndMakeVisible(syncButton);

    // BPM slider - controls internal tempo when not synced
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bpmLabel);

    bpmSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    bpmSlider.setRange(30.0, 300.0, 1.0);
    bpmSlider.setValue(randomWalkProcessor.getInternalBpm());
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    bpmSlider.onValueChange = [this] {
        randomWalkProcessor.setInternalBpm(bpmSlider.getValue());
    };
    bpmSlider.setEnabled(true);
    addAndMakeVisible(bpmSlider);

    // Manual Step toggle - enables step on/off control
    manualStepLabel.setText("Manual Step", juce::dontSendNotification);
    manualStepLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(manualStepLabel);

    manualStepToggle.setToggleState(randomWalkProcessor.isManualStepMode(), juce::dontSendNotification);
    manualStepToggle.onClick = [this] {
        bool isManual = manualStepToggle.getToggleState();
        randomWalkProcessor.setManualStepMode(isManual);
        updateDensitySliderState();
    };
    addAndMakeVisible(manualStepToggle);

    // Initial state update for density slider
    updateDensitySliderState();

    // Step display - visual representation of sequence
    addAndMakeVisible(stepDisplay);
    stepDisplay.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);

    // Set up timer to refresh UI
    startTimerHz(10);

    // Set initial size
    setSize(600, 400);

    DEBUG_LOG("Editor constructor end");
}

/**
 * Updates the density slider enabled state based on manual step mode
 * Disables density control when in manual step mode
 */
void RandomWalkSequencerEditor::updateDensitySliderState()
{
    bool isManualMode = randomWalkProcessor.isManualStepMode();
    densitySlider.setEnabled(!isManualMode);
    densityLabel.setAlpha(isManualMode ? 0.5f : 1.0f);
}

/**
 * Destructor - stops timer and cleans up resources
 */
RandomWalkSequencerEditor::~RandomWalkSequencerEditor()
{
    stopTimer();
}

/**
 * Paints the editor background and header text
 */
void RandomWalkSequencerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawText("Random Walk Sequencer", getLocalBounds(), juce::Justification::centredTop, true);
}

/**
 * Positions all UI components within the editor
 * Called when the component is resized or created
 */
void RandomWalkSequencerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Calculate the total height needed for all controls
    int totalHeight = 40 + 150 + 30 + 10 + (40 + 10) * 7; // Added +1 to account for manual step toggle

    // Set a minimum size for the editor
    setSize(juce::jmax(600, getWidth()), juce::jmax(totalHeight, getHeight()));

    // Reset area after possibly resizing
    area = getLocalBounds().reduced(10);

    // Header section
    auto headerArea = area.removeFromTop(40);

    // Add pattern selector to the left
    auto patternArea = headerArea.removeFromLeft(200);
    patternTypeLabel.setBounds(patternArea.removeFromLeft(80));
    patternTypeComboBox.setBounds(patternArea);

    // Add buttons to the right
    auto buttonArea = headerArea.removeFromRight(240);
    auto buttonWidth = buttonArea.getWidth() / 3;
    randomizeButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    monoButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    playButton.setBounds(buttonArea);

    // Step display
    auto displayArea = area.removeFromTop(150);
    stepDisplay.setBounds(displayArea);

    // Place manual step toggle right below the step display for visibility
    auto manualStepArea = area.removeFromTop(30);
    manualStepLabel.setBounds(manualStepArea.removeFromLeft(80));
    manualStepToggle.setBounds(manualStepArea.removeFromLeft(30));

    area.removeFromTop(10); // Add spacing

    // Transport sync toggle
    syncButton.setBounds(area.removeFromTop(30));

    area.removeFromTop(10); // Add spacing

    // BPM slider - position it to the left side with vertical orientation
    auto bpmArea = area.removeFromLeft(80);
    bpmLabel.setBounds(bpmArea.removeFromTop(20));
    bpmSlider.setBounds(bpmArea.withHeight(100));

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
    rootSlider.setBounds(rootArea.withWidth(juce::jmax(50, rootArea.getWidth() - 60))); // Make room for transpose buttons

    // Add transpose buttons next to root slider
    auto transposeWidth = 30;
    auto transposeHeight = 20;
    transposeUpButton.setBounds(rootArea.getRight() - transposeWidth, rootArea.getY(),
                               transposeWidth, transposeHeight);
    transposeDownButton.setBounds(rootArea.getRight() - transposeWidth, rootArea.getY() + transposeHeight,
                                 transposeWidth, transposeHeight);

    // OR, if you prefer the buttons on a new row:
    area.removeFromTop(10); // Spacing

    // Transpose octave controls
    auto transposeArea = area.removeFromTop(controlHeight);
    transposeLabel.setBounds(transposeArea.removeFromLeft(120)); // Using wider label
    auto transposeBtnWidth = 30;
    transposeDownButton.setBounds(transposeArea.removeFromLeft(transposeBtnWidth));
    transposeArea.removeFromLeft(5); // Small gap between buttons
    transposeUpButton.setBounds(transposeArea.removeFromLeft(transposeBtnWidth));

    // Debug print to see if we have enough space
    DEBUG_LOG("Remaining area height: " << area.getHeight());
}

/**
 * Timer callback to update UI state from the processor
 * Refreshes controls and display at regular intervals
 */
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

    if (static_cast<int>(rootSlider.getValue()) != randomWalkProcessor.getRoot())
    {
        int newValue = randomWalkProcessor.getRoot();
        // Ensure the value is within the slider's range
        newValue = juce::jlimit((int)rootSlider.getMinimum(),
                               (int)rootSlider.getMaximum(),
                               newValue);
        rootSlider.setValue(newValue);
    }

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

/**
 * Updates the manual step toggle button state
 * @param state New state for the toggle button
 */
void RandomWalkSequencerEditor::updateManualStepToggle(bool state)
{
    manualStepToggle.setToggleState(state, juce::sendNotification);
    // Using sendNotification instead of dontSendNotification will trigger the onChange callback
    // which will update the processor and the density slider state
}

/**
 * Constructor for the step display component
 * @param proc Reference to the RandomWalkSequencer processor
 * @param ed Reference to the editor component
 */
RandomWalkSequencerEditor::StepDisplay::StepDisplay(RandomWalkSequencer& proc, RandomWalkSequencerEditor& ed)
    : processor(proc), editor(ed)
{
    // Enable mouse events only
    setInterceptsMouseClicks(true, true);

    // Make cursor change to indicate editable area
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

/**
 * Determines which step was clicked based on mouse position
 * @param e Mouse event containing position information
 * @return Step index (0-15)
 */
int RandomWalkSequencerEditor::StepDisplay::getStepNumberFromMousePosition(const juce::MouseEvent& e)
{
    const int numSteps = 16;
    const float w = (float)getWidth() / numSteps;

    // Calculate which step was clicked
    int stepNumber = (int)(e.position.x / w);

    // Ensure valid range
    return juce::jlimit(0, numSteps - 1, stepNumber);
}

/**
 * Converts vertical position to note value
 * @param y Vertical position in pixels
 * @return Note value offset (-12 to +12)
 */
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

/**
 * Handles mouse button press on a step
 * Starts step value editing
 */
void RandomWalkSequencerEditor::StepDisplay::mouseDown(const juce::MouseEvent& e)
{
    // Identify which step was clicked
    draggedStep = getStepNumberFromMousePosition(e);
}

/**
 * Handles mouse drag to adjust step values
 * Updates the step value based on vertical position
 */
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

/**
 * Handles mouse button release
 * Finalizes step value editing
 */
void RandomWalkSequencerEditor::StepDisplay::mouseUp(const juce::MouseEvent& /*e*/)
{
    // Reset dragged step
    draggedStep = -1;
}

/**
 * Handles double-click to toggle step enabled/disabled
 * Activates manual step mode if not already active
 */
void RandomWalkSequencerEditor::StepDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Identify which step was double-clicked
    int stepNumber = getStepNumberFromMousePosition(e);

    // Toggle the step's enabled state
    processor.toggleStepEnabled(stepNumber);

    // If we're not in manual mode, enable it and update the checkbox
    if (!processor.isManualStepMode())
    {
        processor.setManualStepMode(true);

        // Update the checkbox in the editor - this is the new part
        editor.updateManualStepToggle(true);
    }

    // Redraw the component
    repaint();
}

/**
 * Draws the step sequence visualization
 * Shows current step, note values, and enabled/disabled states
 */
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
        bool isManualMode = processor.isManualStepMode();

        // Get the current step (un-offset)
        int baseCurrentStep = processor.getCurrentStep();

        // Calculate the actual playing step with offset
        int actualCurrentStep = (baseCurrentStep + currentOffset) % numSteps;

        // Draw steps
        for (int i = 0; i < numSteps; ++i)
        {
            // Determine if this step is active (will produce sound)
            bool isActive;

            if (isManualMode) {
                // In Manual Step mode: step is active if it's enabled
                isActive = processor.isStepEnabled(i);
            } else {
                // In Density mode: step is active if it's within the density range
                isActive = false;
                for (int step = 0; step < currentDensity; step++) {
                    int loopStep = (currentOffset + step) % numSteps;
                    if (i == loopStep) {
                        isActive = true;
                        break;
                    }
                }
            }

            // Determine if this is the current playing step
            bool isCurrent = (i == actualCurrentStep);
            bool isBeingDragged = (i == draggedStep);

            // Draw step rectangle
            juce::Rectangle<float> stepRect(i * w, 0, w - 2, h);

            // Color based on step status - always use same colors
            // regardless of mode (manual or density-based)
            if (isBeingDragged) {
                g.setColour(juce::Colours::brown);  // Dragged steps
            } else if (isCurrent && isActive) {
                g.setColour(juce::Colours::orange);  // Current step that's active
            } else if (isCurrent && !isActive) {
                // Current step that's inactive - make it visibly different
                g.setColour(juce::Colours::darkgrey.brighter(0.3f));
            } else if (isActive) {
                g.setColour(juce::Colours::lightgreen);  // Active steps always green
            } else {
                g.setColour(juce::Colours::grey);  // Inactive steps always grey
            }

            g.fillRect(stepRect);

            // Draw note value as a line
            int noteOffset = processor.getSequenceValue(i);
            float lineY = midPoint - (noteOffset * (h / 24.0f)); // Scale to fit in view

            // Draw the note line with a different color when inactive
            if (!isActive) {
                // Dimmed line for inactive steps
                g.setColour(juce::Colours::darkgrey.brighter(0.2f));
                g.drawLine(i * w, lineY, (i + 1) * w - 2, lineY, 1.0f);
            } else {
                // Normal line for active steps
                g.setColour(juce::Colours::white);
                g.drawLine(i * w, lineY, (i + 1) * w - 2, lineY, isBeingDragged ? 3.0f : 2.0f);
            }

            // Draw note value text
            g.setFont(12.0f);
            g.setColour(juce::Colours::white);  // Always use white for text to ensure readability
            g.drawText(juce::String(noteOffset),
                      stepRect.reduced(2),
                      juce::Justification::topLeft,
                      true);

            // Draw step number for clarity
            g.setFont(10.0f);
            g.drawText(juce::String(i + 1), // Add 1 to the index
                       stepRect.reduced(2),
                       juce::Justification::bottomRight,
                       true);

            // In manual mode, add a visual indicator for disabled steps (X pattern)
            if (isManualMode && !isActive) {
                g.setColour(juce::Colours::darkgrey.brighter(0.4f));
                g.drawLine(i * w, 0, (i + 1) * w - 2, h, 1.0f); // Diagonal line to indicate disabled
                g.drawLine(i * w, h, (i + 1) * w - 2, 0, 1.0f); // Other diagonal
            }
        }

        // Draw center line (for reference)
        g.setColour(juce::Colours::darkgrey.brighter(0.3f));
        g.drawLine(0, midPoint, getWidth(), midPoint, 1.0f);

        // Add a label to indicate manual mode
        if (isManualMode) {
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            g.drawText("Manual Step Mode",
                      juce::Rectangle<float>(0, 0, 150, 25),
                      juce::Justification::centredLeft,
                      true);
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("Exception in paint: " << e.what());
    }
    catch (...) {
        DEBUG_LOG("Unknown exception in paint");
    }
}

/**
 * Updates the root note display text to show note name
 * Converts MIDI note number to note name with octave
 */
void RandomWalkSequencerEditor::updateRootNoteDisplay()
{
    int value = randomWalkProcessor.getRoot();

    // Update note name display
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int noteIndex = value % 12;
    int octave = value / 12 - 1;  // MIDI note 60 is C4
    juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
    rootSlider.setTextValueSuffix(" (" + noteName + ")");
}