#pragma once

#include <JuceHeader.h>

// Forward declaration
class RandomWalkSequencerEditor;

/**
 * Main sequencer class that implements a MIDI step sequencer with random walk capabilities
 * Generates MIDI notes based on various step patterns and settings
 */
class RandomWalkSequencer : public juce::AudioProcessor
{
public:
    /**
     * Constructor - initializes the sequencer with default parameters
     */
    RandomWalkSequencer();

    /**
     * Destructor - cleans up any allocated resources
     */
    ~RandomWalkSequencer() override;

    //==============================================================================
    // AudioProcessor methods

    /**
     * Prepares the sequencer for playback
     * Initializes timing parameters based on sample rate
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    /**
     * Releases resources when the sequencer is no longer needed
     * Ensures no hanging MIDI notes remain
     */
    void releaseResources() override;

    /**
     * Main processing method - generates MIDI notes based on the current sequence
     */
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /**
     * Checks if the provided bus layout is compatible with this sequencer
     */
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    //==============================================================================
    // Editor methods

    /**
     * Creates the editor component for the sequencer UI
     */
    juce::AudioProcessorEditor* createEditor() override;

    /**
     * Indicates whether this sequencer has a custom editor
     */
    bool hasEditor() const override;

    //==============================================================================
    // Plugin properties

    /**
     * Returns the name of the sequencer
     */
    const juce::String getName() const override;

    /**
     * Indicates if the sequencer can receive MIDI input
     */
    bool acceptsMidi() const override { return true; } // Match NEEDS_MIDI_INPUT=TRUE

    /**
     * Indicates if the sequencer outputs MIDI messages
     */
    bool producesMidi() const override { return true; } // Match NEEDS_MIDI_OUTPUT=TRUE

    /**
     * Indicates if this is a MIDI effect (rather than an audio processor)
     */
    bool isMidiEffect() const override { return true; } // Since it's a MIDI processor

    /**
     * Returns the tail time of the sequencer in seconds
     */
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    // Program handling

    /**
     * Returns the number of programs (presets) available
     */
    int getNumPrograms() override { return 1; }

    /**
     * Returns the current program (preset) index
     */
    int getCurrentProgram() override { return 0; }

    /**
     * Sets the current program (preset) to the specified index
     */
    void setCurrentProgram(int) override {}

    /**
     * Gets the name of the specified program (preset)
     */
    const juce::String getProgramName(int) override { return {}; }

    /**
     * Changes the name of the specified program (preset)
     */
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    // State handling

    /**
     * Saves the current state of the sequencer to the provided memory block
     */
    void getStateInformation(juce::MemoryBlock& destData) override;

    /**
     * Restores the sequencer state from the provided data
     */
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter access methods

    /**
     * Gets the rate parameter value (step timing)
     */
    int getRate() const;

    /**
     * Returns the rate value in seconds
     */
    float getRateInSeconds() const;

    /**
     * Gets the density parameter value (number of active steps)
     */
    int getDensity() const;

    /**
     * Gets the offset parameter value (sequence start position)
     */
    int getOffset() const;

    /**
     * Gets the gate parameter value (note duration)
     */
    float getGate() const;

    /**
     * Gets the root note parameter value (base MIDI note)
     */
    int getRoot() const;

    /**
     * Sets the rate parameter (step timing)
     */
    void setRate(int value);

    /**
     * Sets the density parameter (number of active steps)
     */
    void setDensity(int value);

    /**
     * Sets the offset parameter (sequence start position)
     */
    void setOffset(int value);

    /**
     * Sets the gate parameter (note duration)
     */
    void setGate(float value);

    /**
     * Sets the root note parameter (base MIDI note)
     */
    void setRoot(int value);

    //==============================================================================
    // Custom methods

    /**
     * Generates a new random sequence based on the selected pattern type
     * @param patternType 0=RandomWalk, 1=Ascending, 2=Descending, 3=Arpeggio
     */
    void randomizeSequence(int patternType = 0);

    /**
     * Generates a random walk pattern sequence
     * Creates musically interesting variations in pitch
     */
    void generateRandomWalk();

    /**
     * Generates an ascending pattern sequence
     */
    void generateAscendingPattern();

    /**
     * Generates a descending pattern sequence
     */
    void generateDescendingPattern();

    /**
     * Generates an arpeggio-style pattern sequence
     */
    void generateArpeggioPattern();

    /**
     * Sets a specific value for a step in the sequence
     */
    void setSequenceValue(int step, int value);

    //==============================================================================
    // Playback control

    /**
     * Starts or stops the sequencer playback
     */
    void setPlaying(bool shouldPlay);

    /**
     * Returns whether the sequencer is currently playing
     */
    bool getIsPlaying() const;

    //==============================================================================
    // Transport sync

    /**
     * Sets whether the sequencer should sync to the host's transport
     */
    void setSyncToHostTransport(bool shouldSync) { syncToHostTransport = shouldSync; }

    //==============================================================================
    // Public accessor methods for StepDisplay

    /**
     * Gets the current step being played
     */
    int getCurrentStep() const { return currentStep; }

    /**
     * Gets the note value for a specific step in the sequence
     */
    int getSequenceValue(int index) const { return sequence[index]; }

    //==============================================================================
    // Manual step control methods

    /**
     * Checks if a specific step is enabled
     */
    bool isStepEnabled(int step) const;

    /**
     * Toggles the enabled state of a specific step
     */
    void toggleStepEnabled(int step);

    /**
     * Enables or disables manual step mode
     * In manual mode, individual steps can be enabled/disabled
     */
    void setManualStepMode(bool isManual);

    /**
     * Returns whether manual step mode is active
     */
    bool isManualStepMode() const { return manualStepMode; }

    /**
     * Resets all steps to enabled state
     */
    void resetEnabledSteps();

    /**
     * Determines if buses can be added to this processor
     */
    bool canAddBus(bool /*isInput*/) const override
    {
        return false;  // Don't allow adding buses
    }

    /**
     * Determines if buses can be removed from this processor
     */
    bool canRemoveBus(bool /*isInput*/) const override
    {
        return false;  // Don't allow removing buses
    }

    /**
     * Indicates if double precision processing is supported
     */
    bool supportsDoublePrecisionProcessing() const override;

    /**
     * Gets the current processing precision
     */
    juce::AudioProcessor::ProcessingPrecision getProcessingPrecision() const;

    /**
     * Processes a block when the processor is bypassed
     */
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    /**
     * Gets whether the sequencer is synced to host transport
     */
    bool getSyncToHostTransport() const { return syncToHostTransport; }

    /**
     * Gets the internal BPM setting
     */
    double getInternalBpm() const { return internalBpm; }

    /**
     * Sets the internal BPM value (used when not synced to host)
     */
    void setInternalBpm(double newBpm);

    /**
     * Transposes the root note up by one octave
     */
    void transposeOctaveUp();

    /**
     * Transposes the root note down by one octave
     */
    void transposeOctaveDown();

    /**
     * Sets all sequence steps to play the root note (mono mode)
     */
    void setMonoMode();

private:

    // Internal BPM setting (used when not synced to host)
    double internalBpm = 120.0;

    // Parameter values
    int rateValue;      // Step timing (note rate)
    int densityValue;   // Number of active steps in the sequence
    int offsetValue;    // Starting position offset in the sequence
    float gateValue;    // Note duration as a proportion of step duration
    int rootValue;      // Base MIDI note number

    // Sequencer properties
    static const int numSteps = 16;       // Total number of steps in the sequence
    int currentStep = 0;                  // Current step being played
    bool isPlaying = false;               // Playback state
    int sequence[numSteps] = {0};         // MIDI note offsets from root note

    // Manual step mode properties
    bool enabledSteps[numSteps] = {true}; // Tracks which steps are enabled
    bool manualStepMode = false;          // Whether manual step mode is active

    // Timing variables
    double sampleRate = 44100.0;          // Current sample rate
    double bpm = 120.0;                   // Current tempo
    double samplesPerBeat = 0.0;          // Number of samples in one beat
    double sampleCounter = 0.0;           // Counter for tracking position within a step
    double stepDuration = 0.0;            // Duration of one step in samples
    int stepOffset = 0;                   // Offset for the current step

    // Note tracking variables
    bool noteIsOn = false;                // Whether a note is currently playing
    int lastNoteValue = 0;                // MIDI note value of the currently playing note

    /**
     * Enhances a sequence to make it more melodically interesting
     */
    void enhanceSequenceMelodically();

    // Transport settings
    bool syncToHostTransport = false; // Whether to sync to host transport

    /**
     * Updates timing based on host information or internal BPM
     */
    void updateTimingInfo();

    /**
     * Gets the MIDI note for the specified step
     */
    int getNoteForStep(int step);

    /**
     * Calculates note length based on gate parameter
     */
    double getNoteLength();

    /**
     * Called when a parameter value changes
     */
    void parameterChanged(const juce::String& parameterID, float newValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomWalkSequencer)
};