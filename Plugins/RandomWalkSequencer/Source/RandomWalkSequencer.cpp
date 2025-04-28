#include <memory>
#include <iostream>
#define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl;

#include "RandomWalkSequencer.h"
#include "RandomWalkSequencerEditor.h"

/**
 * Constructor - initializes the sequencer with default parameters
 * Sets up stereo MIDI buses and initializes the sequence pattern
 */
RandomWalkSequencer::RandomWalkSequencer()
    : AudioProcessor(BusesProperties()
                     .withInput("MIDI In", juce::AudioChannelSet::stereo())
                     .withOutput("MIDI Out", juce::AudioChannelSet::stereo()))
{
    // Set up parameter values manually
    rateValue = 3;       // Default to quarter notes (1/4)
    densityValue = 8;    // Default to 8 steps
    offsetValue = 0;     // Default to no offset
    gateValue = 0.5f;    // Default gate time 50%
    rootValue = 72;      // Default to C5

    // Initialize timing variables
    sampleRate = 44100.0;
    bpm = 120.0;
    internalBpm = 120.0; // Initialize internal BPM

    // Initialize all steps to enabled
    for (int i = 0; i < numSteps; ++i)
    {
        enabledSteps[i] = true;
    }

    // Calculate timing values
    updateTimingInfo();

    // Generate initial sequence
    generateRandomWalk();

    DEBUG_LOG("Processor created with random walk pattern");
}

/**
 * Destructor - cleans up resources
 */
RandomWalkSequencer::~RandomWalkSequencer()
{

}

/**
 * Checks if a step is enabled in manual step mode
 * @param step The step index to check
 * @return True if step is enabled, false otherwise
 */
bool RandomWalkSequencer::isStepEnabled(int step) const
{
    if (step >= 0 && step < numSteps)
    {
        return enabledSteps[step];
    }
    return false;
}

/**
 * Toggles a step's enabled state in manual step mode
 * @param step The step index to toggle
 */
void RandomWalkSequencer::toggleStepEnabled(int step)
{
    if (step >= 0 && step < numSteps)
    {
        enabledSteps[step] = !enabledSteps[step];
    }
}

/**
 * Sets whether manual step mode is active
 * In manual mode, individual steps can be enabled/disabled
 */
void RandomWalkSequencer::setManualStepMode(bool isManual)
{
    manualStepMode = isManual;

    // If we're disabling manual mode, reset all steps to enabled
    if (!isManual)
    {
        resetEnabledSteps();
    }
}

/**
 * Resets all steps to enabled state
 * Called when exiting manual step mode
 */
void RandomWalkSequencer::resetEnabledSteps()
{
    // Reset all steps to enabled
    for (int i = 0; i < numSteps; ++i)
    {
        enabledSteps[i] = true;
    }
}

/**
 * Prepares the sequencer for playback
 * Initializes timing parameters based on sample rate
 */
void RandomWalkSequencer::prepareToPlay(double sampleRateToUse, int /*samplesPerBlock*/)
{
    this->sampleRate = sampleRateToUse;

    // Reset playback state
    currentStep = 0;
    sampleCounter = 0.0;
    noteIsOn = false;

    // Initialize timing information
    updateTimingInfo();

    DEBUG_LOG("prepareToPlay called, sampleRate = " << sampleRateToUse);
}

/**
 * Releases resources when the sequencer is no longer needed
 * Ensures no hanging MIDI notes remain
 */
void RandomWalkSequencer::releaseResources()
{
    // Turn off sequencer when the plugin is deactivated
    isPlaying = false;

    // Make sure no notes are left on
    if (noteIsOn)
    {
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
        juce::MidiBuffer tempBuffer;
        tempBuffer.addEvent(noteOff, 0);
        noteIsOn = false;
    }
}

/**
 * Indicates if double precision processing is supported
 * This sequencer only uses single precision
 */
bool RandomWalkSequencer::supportsDoublePrecisionProcessing() const
{
    return false;
}

/**
 * Gets the current processing precision
 * Always returns single precision for this sequencer
 */
juce::AudioProcessor::ProcessingPrecision RandomWalkSequencer::getProcessingPrecision() const
{
    return juce::AudioProcessor::singlePrecision;
}

/**
 * Processes a block when the processor is bypassed
 * Simply clears the audio buffer since this is a MIDI effect
 */
void RandomWalkSequencer::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    buffer.clear();
}

/**
 * Main processing method - generates MIDI notes based on the current sequence
 * Handles timing, note generation, and step advancement
 */
void RandomWalkSequencer::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Update timing info at the start of each block to keep in sync with host transport
    updateTimingInfo();

    // Debug log to check if we're getting called with MIDI data
    if (!midiMessages.isEmpty())
    {
        DEBUG_LOG("Received MIDI data: " << midiMessages.getNumEvents() << " events");
    }

    // Debug log to check transport sync
    if (isPlaying && samplesPerBeat > 0)
    {
        static int callCount = 0;
        if (++callCount % 100 == 0)  // Don't log every buffer to avoid flooding
            DEBUG_LOG("Plugin is playing, BPM: " << bpm << ", step: " << currentStep);
    }

    // Clear audio buffer since this is a MIDI effect only
    buffer.clear();

    // Get buffer size
    auto numSamples = buffer.getNumSamples();

    // Create a MIDI buffer for our output
    juce::MidiBuffer processedMidi;

    // Pass through any incoming MIDI messages to our processed MIDI buffer
    for (const auto& metadata : midiMessages)
    {
        processedMidi.addEvent(metadata.getMessage(), metadata.samplePosition);
    }

    // Process our sequencer if we're properly initialized
    if (sampleRate > 0.0 && stepDuration > 0.0 && isPlaying)
    {
        // Track the time within this buffer
        int samplePosition = 0;

        while (samplePosition < numSamples)
        {
            // Check if we need to advance to next step
            if (sampleCounter >= stepDuration)
            {
                // Reset the sample counter for the next step
                sampleCounter -= stepDuration;

                // Turn off previous note if it's still on
                if (noteIsOn)
                {
                    auto noteOffMessage = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                    processedMidi.addEvent(noteOffMessage, samplePosition);
                    noteIsOn = false;
                }

                // Advance to the next step based on mode
                if (manualStepMode)
                {
                    // In Manual Step mode: All 16 steps are looped through,
                    // but only enabled steps produce sound
                    currentStep = (currentStep + 1) % numSteps;
                }
                else
                {
                    // In Density mode: Only steps within density range are looped
                    currentStep = (currentStep + 1) % densityValue;
                }

                // Calculate the actual step index in the sequence, considering offset
                int actualStepIndex = (currentStep + offsetValue) % numSteps;

                // Determine if we should play a note for this step
                bool shouldPlayNote;

                if (manualStepMode)
                {
                    // In Manual Step mode: Only play if step is enabled
                    shouldPlayNote = enabledSteps[actualStepIndex];
                }
                else
                {
                    // In Density mode: Always play the steps in range
                    shouldPlayNote = true;
                }

                if (shouldPlayNote)
                {
                    // Calculate the MIDI note for this step
                    int noteValue = getNoteForStep(actualStepIndex);

                    // Send note on message with velocity based on step
                    juce::uint8 velocity = 80 + (juce::uint8)(30.0 * std::abs(sequence[actualStepIndex]) / 12.0);
                    auto noteOnMessage = juce::MidiMessage::noteOn(1, noteValue, velocity);
                    processedMidi.addEvent(noteOnMessage, samplePosition);

                    // Log the note played
                    DEBUG_LOG("Playing note " << noteValue << " at step " << actualStepIndex);

                    // Remember this note and that we've turned it on
                    lastNoteValue = noteValue;
                    noteIsOn = true;
                }
            }

            // Determine how many samples to process next
            auto samplesThisSegment = juce::jmin(numSamples - samplePosition,
                                              (int) (stepDuration - sampleCounter));

            // Check if we need to turn off the note based on gate time
            if (noteIsOn && (sampleCounter + samplesThisSegment >= getNoteLength()))
            {
                // Calculate exact sample position for note off
                auto noteOffPosition = samplePosition + (int) (getNoteLength() - sampleCounter);

                // Ensure we don't go outside the buffer
                noteOffPosition = juce::jmin(noteOffPosition, numSamples - 1);

                // Send note off message
                auto noteOffMessage = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                processedMidi.addEvent(noteOffMessage, noteOffPosition);

                noteIsOn = false;
            }

            // Protect against impossible values to prevent crashes
            if (samplesThisSegment <= 0) {
                DEBUG_LOG("Warning: samplesThisSegment <= 0, resetting to 1");
                samplesThisSegment = 1;
            }

            // Advance our counters
            sampleCounter += samplesThisSegment;
            samplePosition += samplesThisSegment;
        }
    }
    else {
        // If we're not playing but have an active note, turn it off
        if (noteIsOn) {
            auto noteOffMessage = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
            processedMidi.addEvent(noteOffMessage, 0);
            noteIsOn = false;
        }
    }

    // Replace original MIDI with our processed MIDI
    midiMessages.swapWith(processedMidi);
}

/**
 * Checks if the provided bus layout is compatible with this sequencer
 * Supports various bus configurations for maximum compatibility
 */
bool RandomWalkSequencer::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // For REAPER compatibility, we'll be more permissive with bus layouts

    // Accept if both input and output are stereo (most common case)
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo())
        return true;

    // Accept if both are mono
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono())
        return true;

    // Accept if both are disabled
    if (layouts.getMainInputChannelSet().isDisabled() &&
        layouts.getMainOutputChannelSet().isDisabled())
        return true;

    // Also accept asymmetric layouts as long as they're valid channel sets
    if (!layouts.getMainInputChannelSet().isDisabled() &&
        !layouts.getMainOutputChannelSet().isDisabled())
        return true;

    return false;
}

//==============================================================================
// Pattern Generation Methods
//==============================================================================

/**
 * Generates an ascending pattern
 * Creates a mostly upward moving melody with occasional downward steps
 */
void RandomWalkSequencer::generateAscendingPattern()
{
    juce::Random random;

    // Start from a low value
    int currentValue = -6;

    // Generate an ascending pattern
    for (int i = 0; i < numSteps; ++i)
    {
        // Add some randomness but mostly ascending
        if (random.nextFloat() < 0.2f)
            currentValue--; // Occasionally go down for interest
        else
            currentValue++;

        // Keep within reasonable range
        if (currentValue < -12) currentValue = -12;
        if (currentValue > 12) currentValue = 12;

        // Store the value
        sequence[i] = currentValue;
    }
}

/**
 * Generates a descending pattern
 * Creates a mostly downward moving melody with occasional upward steps
 */
void RandomWalkSequencer::generateDescendingPattern()
{
    juce::Random random;

    // Start from a high value
    int currentValue = 6;

    // Generate a descending pattern
    for (int i = 0; i < numSteps; ++i)
    {
        // Add some randomness but mostly descending
        if (random.nextFloat() < 0.2f)
            currentValue++; // Occasionally go up for interest
        else
            currentValue--;

        // Keep within reasonable range
        if (currentValue < -12) currentValue = -12;
        if (currentValue > 12) currentValue = 12;

        // Store the value
        sequence[i] = currentValue;
    }
}

/**
 * Generates an arpeggio pattern
 * Creates a sequence based on chord tones (major chord by default)
 */
void RandomWalkSequencer::generateArpeggioPattern()
{
    // Define some musical intervals (semitones)
    const int intervals[] = { 0, 4, 7, 12 }; // Major chord: root, major third, perfect fifth, octave
    const int numIntervals = 4;

    juce::Random random;

    for (int i = 0; i < numSteps; ++i)
    {
        // Choose a random interval from our chord
        int intervalIndex = random.nextInt(numIntervals);
        int value = intervals[intervalIndex];

        // Occasionally invert down an octave for bass notes
        if (random.nextFloat() < 0.3f && value > 0)
            value -= 12;

        sequence[i] = value;
    }
}

/**
 * Saves the current state of the sequencer to the provided memory block
 * Stores all parameters and sequence data as XML
 */
void RandomWalkSequencer::getStateInformation(juce::MemoryBlock& destData)
{
    // Create XML to store parameter values
    juce::XmlElement xml("RandomWalkSequencerState");

    // Add parameters
    xml.setAttribute("rate", rateValue);
    xml.setAttribute("density", densityValue);
    xml.setAttribute("offset", offsetValue);
    xml.setAttribute("gate", gateValue);
    xml.setAttribute("root", rootValue);
    xml.setAttribute("manualStepMode", manualStepMode);

    // Add sequence data
    juce::XmlElement* sequenceXml = xml.createNewChildElement("Sequence");
    for (int i = 0; i < numSteps; ++i)
    {
        sequenceXml->setAttribute("Step" + juce::String(i), sequence[i]);
        sequenceXml->setAttribute("Enabled" + juce::String(i), enabledSteps[i]);
    }

    // Write to binary
    copyXmlToBinary(xml, destData);
    DEBUG_LOG("State saved");
}

/**
 * Restores the sequencer state from the provided data
 * Loads all parameters and sequence data from XML
 */
void RandomWalkSequencer::setStateInformation(const void* data, int sizeInBytes)
{
    // Parse XML from binary
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName("RandomWalkSequencerState"))
    {
        // Restore parameters
        rateValue = xmlState->getIntAttribute("rate", 1);
        densityValue = xmlState->getIntAttribute("density", 16);
        offsetValue = xmlState->getIntAttribute("offset", 0);
        gateValue = static_cast<float>(xmlState->getDoubleAttribute("gate", 0.5));
        rootValue = xmlState->getIntAttribute("root", 72);  // Changed from 60 to 72
        manualStepMode = xmlState->getBoolAttribute("manualStepMode", false);
        internalBpm = xmlState->getDoubleAttribute("internalBpm", 120.0); // Restore internal BPM

        // Restore sequence data
        juce::XmlElement* sequenceXml = xmlState->getChildByName("Sequence");
        if (sequenceXml != nullptr)
        {
            for (int i = 0; i < numSteps; ++i)
            {
                if (sequenceXml->hasAttribute("Step" + juce::String(i)))
                {
                    sequence[i] = sequenceXml->getIntAttribute("Step" + juce::String(i));
                }

                if (sequenceXml->hasAttribute("Enabled" + juce::String(i)))
                {
                    enabledSteps[i] = sequenceXml->getBoolAttribute("Enabled" + juce::String(i), true);
                }
            }
        }

        DEBUG_LOG("State restored");
    }
}

/**
 * Creates the editor component for the sequencer UI
 * @return A new editor instance
 */
juce::AudioProcessorEditor* RandomWalkSequencer::createEditor()
{
    // Create the editor without triggering any sequence regeneration
    return new RandomWalkSequencerEditor(*this);
}

/**
 * Indicates whether this sequencer has a custom editor
 * @return Always returns true
 */
bool RandomWalkSequencer::hasEditor() const
{
    return true;
}

/**
 * Called when a parameter value changes (unused in this implementation)
 */
void RandomWalkSequencer::parameterChanged(const juce::String&, float)
{
    // Not used without AudioProcessorValueTreeState
}

//==============================================================================
// Parameter access methods
//==============================================================================

/**
 * Gets the rate parameter value (step timing)
 */
int RandomWalkSequencer::getRate() const { return rateValue; }

/**
 * Gets the density parameter value (number of active steps)
 */
int RandomWalkSequencer::getDensity() const { return densityValue; }

/**
 * Gets the offset parameter value (sequence start position)
 */
int RandomWalkSequencer::getOffset() const { return offsetValue; }

/**
 * Gets the gate parameter value (note duration)
 */
float RandomWalkSequencer::getGate() const { return gateValue; }

/**
 * Gets the root note parameter value (base MIDI note)
 */
int RandomWalkSequencer::getRoot() const { return rootValue; }

/**
 * Sets the rate parameter (step timing)
 * Updates timing information when changed
 */
void RandomWalkSequencer::setRate(int value) { rateValue = value; updateTimingInfo(); }

/**
 * Sets the density parameter (number of active steps)
 * Resets current step if it's now outside the loop range
 */
void RandomWalkSequencer::setDensity(int value)
{
    // Only update if value changed
    if (densityValue != value) {
        densityValue = value;

        // Reset currentStep if it's now outside the loop range
        if (currentStep >= densityValue) {
            currentStep = 0;
        }
    }
}

/**
 * Sets the offset parameter (sequence start position)
 */
void RandomWalkSequencer::setOffset(int value) { offsetValue = value; }

/**
 * Sets the gate parameter (note duration)
 */
void RandomWalkSequencer::setGate(float value) { gateValue = value; }

/**
 * Sets the root note parameter (base MIDI note)
 */
void RandomWalkSequencer::setRoot(int value) { rootValue = value; }

//==============================================================================
// Core Sequencer Functionality
//==============================================================================

/**
 * Randomizes the sequence based on the selected pattern type
 * @param patternType 0=RandomWalk, 1=Ascending, 2=Descending, 3=Arpeggio
 */
void RandomWalkSequencer::randomizeSequence(int patternType)
{
    // Save the current enabled states if in manual mode
    bool savedEnabledStates[numSteps];
    if (manualStepMode)
    {
        for (int i = 0; i < numSteps; ++i)
        {
            savedEnabledStates[i] = enabledSteps[i];
        }
    }

    switch (patternType)
    {
        case 0: // Random walk
            generateRandomWalk();
        break;

        case 1: // Ascending
            generateAscendingPattern();
        break;

        case 2: // Descending
            generateDescendingPattern();
        break;

        case 3: // Arpeggio
            generateArpeggioPattern();
        break;

        default:
            generateRandomWalk();
    }

    // Restore the enabled states if in manual mode
    if (manualStepMode)
    {
        for (int i = 0; i < numSteps; ++i)
        {
            enabledSteps[i] = savedEnabledStates[i];
        }
    }

    // Notify that sequence has changed (useful for GUI updates)
    if (auto* editor = dynamic_cast<RandomWalkSequencerEditor*>(getActiveEditor()))
        editor->repaint();
}

/**
 * Starts or stops the sequencer playback
 * Handles note cleanup when starting/stopping
 */
void RandomWalkSequencer::setPlaying(bool shouldPlay)
{
    // Only update if the state is actually changing
    if (isPlaying != shouldPlay)
    {
        isPlaying = shouldPlay;

        // If starting playback, reset counters and stop any hanging notes
        if (isPlaying)
        {
            sampleCounter = 0.0;
            currentStep = numSteps - 1; // Will increment to 0 on first step

            // Make sure no notes are left on
            if (noteIsOn)
            {
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                juce::MidiBuffer tempBuffer;
                tempBuffer.addEvent(noteOff, 0);
                noteIsOn = false;
            }
        }
    }
    if (!shouldPlay && noteIsOn)
    {
        // Send a note off message immediately when stopping
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
        juce::MidiBuffer tempBuffer;
        tempBuffer.addEvent(noteOff, 0);
        noteIsOn = false;
    }
}

/**
 * Sets a specific value for a step in the sequence
 * @param step The step index to modify
 * @param value The new note value (-12 to +12 semitones from root)
 */
void RandomWalkSequencer::setSequenceValue(int step, int value)
{
    // Ensure step is in valid range
    if (step >= 0 && step < numSteps)
    {
        // Limit value to reasonable range (-12 to +12 semitones)
        value = juce::jlimit(-12, 12, value);

        // Update the sequence
        sequence[step] = value;
    }
}

/**
 * Returns whether the sequencer is currently playing
 * @return Current playback state
 */
bool RandomWalkSequencer::getIsPlaying() const
{
    return isPlaying;
}

/**
 * Returns the name of the sequencer
 */
const juce::String RandomWalkSequencer::getName() const
{
    return "RandomWalkSequencer";
}

/**
 * Updates timing information based on BPM and rate settings
 * Handles host transport sync if enabled
 */
void RandomWalkSequencer::updateTimingInfo()
{
    auto* playHead = getPlayHead();

    // Reset sample counter at appropriate moments to ensure tight sync
    double oldBpm = bpm;

    if (playHead != nullptr && syncToHostTransport)
    {
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo = playHead->getPosition();

        if (posInfo.hasValue())
        {
            // Update BPM from host if available and synced
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();

            // Only control playback if we're synced to host
            bool hostIsPlaying = posInfo->getIsPlaying();

            // This section is crucial - make sure to get the correct playing state
            if (hostIsPlaying && !isPlaying)
            {
                // Debug info - helps track down sync issues
                DEBUG_LOG("Host started playing - starting sequencer");

                // Start the sequencer
                isPlaying = true;
                currentStep = numSteps - 1; // Will increment to 0 on first step
                sampleCounter = 0.0;

                // Make sure no notes are left on
                if (noteIsOn)
                {
                    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                    juce::MidiBuffer tempBuffer;
                    tempBuffer.addEvent(noteOff, 0);
                    noteIsOn = false;
                }
            }
            else if (!hostIsPlaying && isPlaying)
            {
                // Debug info
                DEBUG_LOG("Host stopped playing - stopping sequencer");

                // Stop the sequencer
                isPlaying = false;

                // Make sure no notes are left on
                if (noteIsOn)
                {
                    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                    juce::MidiBuffer tempBuffer;
                    tempBuffer.addEvent(noteOff, 0);
                    noteIsOn = false;
                }
            }
        }
    }
    else if (!syncToHostTransport)
    {
        // When not synced to host, use internal BPM
        bpm = internalBpm;
    }

    // Check for BPM changes and reset sample counter if needed
    if (std::abs(oldBpm - bpm) > 0.01)
    {
        DEBUG_LOG("BPM changed from " << oldBpm << " to " << bpm);
        sampleCounter = 0.0;
    }

    // Calculate timing values
    samplesPerBeat = (60.0 / bpm) * sampleRate;
    stepDuration = samplesPerBeat * getRateInSeconds();

    // Debug timing values
    if (isPlaying) {
        static int debugCounter = 0;
        if (++debugCounter % 100 == 0) { // Limit debug output
            DEBUG_LOG("Timing: BPM=" << bpm << ", samplesPerBeat=" << samplesPerBeat
                      << ", stepDuration=" << stepDuration);
        }
    }
}

/**
 * Converts rate parameter to actual timing value in beats
 * @return Duration of one step in beats (e.g. 0.25 = quarter note)
 */
float RandomWalkSequencer::getRateInSeconds() const
{
    // Convert rate parameter to actual timing value
    const float rateValues[] = { 1.0f/32.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/3.0f, 1.0f/2.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    return rateValues[rateValue];
}

/**
 * Generates a random walk pattern sequence
 * Creates musically interesting variations in pitch
 */
void RandomWalkSequencer::generateRandomWalk()
{
    juce::Random random;

    // Parameters for enhanced random walk with much more variability
    const int maxJump = 7;              // Increased maximum basic step size
    const int maxRange = 12;            // Maximum range (one octave)
    const float stayProb = 0.05f;       // Reduced probability to stay on same note
    const float bigJumpProb = 0.25f;    // Increased probability for larger jumps
    const float patternBreakProb = 0.1f; // Probability to break a pattern completely
    const float resetProb = 0.05f;      // Probability to reset to center

    // Start from a random point rather than always the middle
    int currentValue = random.nextInt(maxRange * 2 + 1) - maxRange;
    sequence[0] = currentValue;

    int prevDirection = 0;
    int consecutiveSteps = 0;

    // Generate pattern with more deliberate changes in direction
    for (int i = 1; i < numSteps; ++i)
    {
        // Occasionally reset to create phrases
        if (random.nextFloat() < resetProb) {
            currentValue = random.nextInt(maxRange * 2 + 1) - maxRange; // Random reset point
            consecutiveSteps = 0;
            prevDirection = 0;
        }
        // Decide if we should break the pattern
        else if (random.nextFloat() < patternBreakProb || consecutiveSteps > 3) {
            // Force a direction change to break monotony
            prevDirection = prevDirection == 0 ? (random.nextBool() ? 1 : -1) : -prevDirection;

            // Make a significant jump to break the pattern
            int jumpSize = 3 + random.nextInt(9); // Jumps of 3 to 12 semitones
            currentValue += prevDirection * jumpSize;
            consecutiveSteps = 0;
        }
        // Stay on same note occasionally
        else if (random.nextFloat() < stayProb) {
            // Do nothing - stay on same note
            consecutiveSteps = 0;
        }
        else {
            // Choose a direction that might be different from previous
            int direction;

            if (consecutiveSteps >= 2 && random.nextFloat() < 0.7f) {
                // After 2+ steps in same direction, higher chance of change
                direction = -prevDirection;
            } else {
                // Random direction with slight bias toward previous
                direction = (random.nextFloat() < 0.4f) ?
                    -prevDirection : (prevDirection != 0 ? prevDirection : (random.nextBool() ? 1 : -1));
            }

            // Determine step size with more variety
            int stepSize;
            if (random.nextFloat() < bigJumpProb) {
                // Larger jumps for more variety
                stepSize = 4 + random.nextInt(maxJump);
            } else {
                // Use different step size distribution
                // Higher probability of 1,2,3 steps, lower for larger steps
                float r = random.nextFloat();
                if (r < 0.5f)
                    stepSize = 1;
                else if (r < 0.8f)
                    stepSize = 2;
                else
                    stepSize = 3 + random.nextInt(maxJump - 2);
            }

            // Apply the step
            currentValue += direction * stepSize;

            // Update tracking variables
            if (direction == prevDirection)
                consecutiveSteps++;
            else {
                prevDirection = direction;
                consecutiveSteps = 1;
            }
        }

        // Keep within range but with soft boundaries
        if (currentValue > maxRange) {
            if (random.nextFloat() < 0.7f) {
                // Usually reflect back
                currentValue = maxRange - (currentValue - maxRange);
                prevDirection = -prevDirection;
            } else {
                // Sometimes just clamp
                currentValue = maxRange;
            }
        } else if (currentValue < -maxRange) {
            if (random.nextFloat() < 0.7f) {
                // Usually reflect back
                currentValue = -maxRange + (-maxRange - currentValue);
                prevDirection = -prevDirection;
            } else {
                // Sometimes just clamp
                currentValue = -maxRange;
            }
        }

        // Store the value
        sequence[i] = currentValue;
    }

    // Add a final pass to ensure melodic interest
    enhanceSequenceMelodically();

    DEBUG_LOG("Random walk sequence generated");
}

/**
 * Helper method to enhance the musical quality of the sequence
 * Breaks up repetitive patterns and adds accents/octave jumps
 */
void RandomWalkSequencer::enhanceSequenceMelodically()
{
    juce::Random random;

    // Find any boring sections (3+ consecutive steps in same direction)
    for (int i = 2; i < numSteps-1; i++) {
        int diff1 = sequence[i] - sequence[i-1];
        int diff2 = sequence[i-1] - sequence[i-2];

        // If we have 3 steps moving in the same direction with same interval
        if (diff1 == diff2 && diff1 != 0) {
            // Break the pattern by adding a jump or change
            if (random.nextBool()) {
                // Reverse direction
                sequence[i+1] = sequence[i] - diff1;
            } else {
                // Make a jump
                sequence[i+1] = sequence[i] + (random.nextBool() ? 3 : -3);
            }
            i++; // Skip the fixed note
        }
    }

    // Create a few accents by adding octave jumps
    int numAccents = 1 + random.nextInt(2); // 1-2 accents
    for (int i = 0; i < numAccents; i++) {
        int pos = 2 + random.nextInt(numSteps - 3); // Not too close to start/end
        // Jump up or down an octave if within range
        int newValue = sequence[pos] + (random.nextBool() ? 12 : -12);
        if (newValue >= -12 && newValue <= 12) {
            sequence[pos] = newValue;
        }
    }
}

/**
 * Calculates the MIDI note value for a specific step
 * @param step The step index
 * @return MIDI note value (root + offset)
 */
int RandomWalkSequencer::getNoteForStep(int step)
{
    // step is already offset-adjusted, so use it directly to access the sequence array
    return rootValue + sequence[step];
}

/**
 * Calculates the duration of a note based on gate time
 * @return Note duration in samples
 */
double RandomWalkSequencer::getNoteLength()
{
    return stepDuration * gateValue;
}

/**
 * Sets the internal BPM (used when not synced to host)
 * @param newBpm The new BPM value
 */
void RandomWalkSequencer::setInternalBpm(double newBpm)
{
    // Limit BPM to a reasonable range
    internalBpm = juce::jlimit(30.0, 300.0, newBpm);

    // Only update timing if we're not synced to host
    if (!syncToHostTransport)
    {
        bpm = internalBpm;
        updateTimingInfo();
    }
}

/**
 * Transposes the root note up by one octave
 * Limited to maximum of C9 (MIDI 120)
 */
void RandomWalkSequencer::transposeOctaveUp()
{
    // Don't transpose above C9 (MIDI note 120)
    if (rootValue <= 108) // C9 - 12 = 108 to ensure we can go up one octave
    {
        rootValue += 12;
        DEBUG_LOG("Transposed up one octave: Root = " << rootValue);
    }
    else
    {
        // Optionally add feedback when limit is reached
        DEBUG_LOG("Cannot transpose higher than C9");
    }
}

/**
 * Transposes the root note down by one octave
 * Limited to minimum of C0 (MIDI 12)
 */
void RandomWalkSequencer::transposeOctaveDown()
{
    // Don't transpose below C0 (MIDI note 12)
    if (rootValue >= 24) // C0 + 12 = 24 to ensure we can go down one octave
    {
        rootValue -= 12;
        DEBUG_LOG("Transposed down one octave: Root = " << rootValue);
    }
    else
    {
        // Optionally add feedback when limit is reached
        DEBUG_LOG("Cannot transpose lower than C0");
    }
}

/**
 * Sets all sequence steps to play the root note
 * Creates a mono/unison sequence pattern
 */
void RandomWalkSequencer::setMonoMode()
{
    // Set all sequence steps to 0 (root note)
    for (int i = 0; i < numSteps; ++i)
    {
        sequence[i] = 0; // 0 means no offset, so it will play the root note
    }

    // If we have an editor, update the display
    if (auto* editor = dynamic_cast<RandomWalkSequencerEditor*>(getActiveEditor()))
        editor->repaint();

    DEBUG_LOG("Set all steps to mono (root note)");
}