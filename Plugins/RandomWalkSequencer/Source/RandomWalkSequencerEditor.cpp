#include <memory>
#include <iostream>
#define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl;

#include "RandomWalkSequencer.h"
#include "RandomWalkSequencerEditor.h"

// Minimal constructor with no AudioProcessorValueTreeState at all
RandomWalkSequencer::RandomWalkSequencer()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo())
                     .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    // Set up parameter values manually
    rateValue = 3;       // Default to quarter notes (1/4)
    densityValue = 8;    // Default to 8 steps
    offsetValue = 0;     // Default to no offset
    gateValue = 0.5f;    // Default gate time 50%
    rootValue = 60;      // Default to C4

    // Initialize timing variables
    sampleRate = 44100.0;
    bpm = 120.0;

    // Initialize all steps to enabled
    for (int i = 0; i < numSteps; ++i)
    {
        enabledSteps[i] = true;
    }

    // Calculate timing values
    updateTimingInfo();

    // Generate initial sequence
    generateRandomWalk();

    // Disable the audio buses for MIDI-only operation
    // This is important for VST3 MIDI effect compatibility
    for (int i = 0; i < getBusCount(true); ++i)
        getBus(true, i)->enable(false);

    for (int i = 0; i < getBusCount(false); ++i)
        getBus(false, i)->enable(false);

    DEBUG_LOG("Processor created with random walk pattern");
}

RandomWalkSequencer::~RandomWalkSequencer()
{

}

// Implementation of new methods for manual step control
bool RandomWalkSequencer::isStepEnabled(int step) const
{
    if (step >= 0 && step < numSteps)
    {
        return enabledSteps[step];
    }
    return false;
}

void RandomWalkSequencer::toggleStepEnabled(int step)
{
    if (step >= 0 && step < numSteps)
    {
        enabledSteps[step] = !enabledSteps[step];
    }
}

void RandomWalkSequencer::setManualStepMode(bool isManual)
{
    manualStepMode = isManual;

    // If we're disabling manual mode, reset all steps to enabled
    if (!isManual)
    {
        resetEnabledSteps();
    }
}

void RandomWalkSequencer::resetEnabledSteps()
{
    // Reset all steps to enabled
    for (int i = 0; i < numSteps; ++i)
    {
        enabledSteps[i] = true;
    }
}

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

bool RandomWalkSequencer::supportsDoublePrecisionProcessing() const
{
    return false;
}

juce::AudioProcessor::ProcessingPrecision RandomWalkSequencer::getProcessingPrecision() const
{
    return juce::AudioProcessor::singlePrecision;
}

void RandomWalkSequencer::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    buffer.clear();
}

void RandomWalkSequencer::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
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

                // Send note off message
                auto noteOffMessage = juce::MidiMessage::noteOff(1, lastNoteValue, (juce::uint8) 0);
                processedMidi.addEvent(noteOffMessage, noteOffPosition);

                noteIsOn = false;
            }

            // Advance our counters
            sampleCounter += samplesThisSegment;
            samplePosition += samplesThisSegment;
        }
    }

    // Replace original MIDI with our processed MIDI
    midiMessages.swapWith(processedMidi);
}

bool RandomWalkSequencer::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // More permissive approach for Ableton:
    // Accept configurations where audio is disabled OR where it has stereo buses
    bool isInputDisabled = layouts.getMainInputChannelSet().isDisabled();
    bool isOutputDisabled = layouts.getMainOutputChannelSet().isDisabled();

    // Accept if both are disabled (MIDI only)
    if (isInputDisabled && isOutputDisabled)
        return true;

    // Also accept stereo/stereo for Ableton compatibility
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo())
        return true;

    return false;
}

// Add more pattern generation algorithms for different musical feels

// Generate an ascending pattern
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

// Generate a descending pattern
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

// Generate a pattern with musical intervals (e.g., arpeggios)
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
        gateValue = xmlState->getDoubleAttribute("gate", 0.5);
        rootValue = xmlState->getIntAttribute("root", 60);
        manualStepMode = xmlState->getBoolAttribute("manualStepMode", false);

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

juce::AudioProcessorEditor* RandomWalkSequencer::createEditor()
{
    return new RandomWalkSequencerEditor(*this);
}

bool RandomWalkSequencer::hasEditor() const
{
    return true;
}

void RandomWalkSequencer::parameterChanged(const juce::String&, float)
{
    // Not used without AudioProcessorValueTreeState
}

// Custom parameter getters/setters
int RandomWalkSequencer::getRate() const { return rateValue; }
int RandomWalkSequencer::getDensity() const { return densityValue; }
int RandomWalkSequencer::getOffset() const { return offsetValue; }
float RandomWalkSequencer::getGate() const { return gateValue; }
int RandomWalkSequencer::getRoot() const { return rootValue; }

void RandomWalkSequencer::setRate(int value) { rateValue = value; updateTimingInfo(); }
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
void RandomWalkSequencer::setOffset(int value) { offsetValue = value; }
void RandomWalkSequencer::setGate(float value) { gateValue = value; }
void RandomWalkSequencer::setRoot(int value) { rootValue = value; }

// Core functionality
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

// Add a method to start/stop the sequencer
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
}

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

// Add accessor for isPlaying state
bool RandomWalkSequencer::getIsPlaying() const
{
    return isPlaying;
}

// Implement the missing JUCE processor callbacks
const juce::String RandomWalkSequencer::getName() const
{
    return "RandomWalkSequencer";
}

// Update the timing information when BPM or time signature changes
void RandomWalkSequencer::updateTimingInfo()
{
    // Get host information if available
    auto* playHead = getPlayHead();

    if (playHead != nullptr)
    {
        // Use the newer API
        juce::Optional<juce::AudioPlayHead::PositionInfo> posInfo = playHead->getPosition();

        if (posInfo.hasValue())
        {
            // Update BPM from host if available
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();

            // Sync with host transport if it's playing
            bool isHostPlaying = posInfo->getIsPlaying();
            if (isHostPlaying && syncToHostTransport && !isPlaying)
                setPlaying(true);
            else if (!isHostPlaying && syncToHostTransport && isPlaying)
                setPlaying(false);
        }
    }

    // Calculate samples per beat
    samplesPerBeat = (60.0 / bpm) * sampleRate;

    // Calculate step duration based on rate
    stepDuration = samplesPerBeat * getRateInSeconds();
}

float RandomWalkSequencer::getRateInSeconds() const
{
    // Convert rate parameter to actual timing value
    const float rateValues[] = { 1.0f/32.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/3.0f, 1.0f/2.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    return rateValues[rateValue];
}

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

    DEBUG_LOG("Enhanced random walk sequence generated with high variability");
}

// Add this helper method to further enhance the sequence
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

int RandomWalkSequencer::getNoteForStep(int step)
{
    // step is already offset-adjusted, so use it directly to access the sequence array
    return rootValue + sequence[step];
}

double RandomWalkSequencer::getNoteLength()
{
    return stepDuration * gateValue;
}

void RandomWalkSequencerEditor::updateManualStepToggle(bool state)
{
    manualStepToggle.setToggleState(state, juce::sendNotification);
    // Using sendNotification instead of dontSendNotification will trigger the onChange callback
    // which will update the processor and the density slider state
}