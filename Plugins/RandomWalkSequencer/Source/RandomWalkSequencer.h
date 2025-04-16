#pragma once

#include <JuceHeader.h>

// Forward declaration
class RandomWalkSequencerEditor;

class RandomWalkSequencer : public juce::AudioProcessor
{
public:
    RandomWalkSequencer();
    ~RandomWalkSequencer() override;

    // AudioProcessor methods
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Editor methods
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // Plugin properties
    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; } // Match NEEDS_MIDI_INPUT=TRUE
    bool producesMidi() const override { return true; } // Match NEEDS_MIDI_OUTPUT=TRUE
    bool isMidiEffect() const override { return true; } // Since it's a MIDI processor
    double getTailLengthSeconds() const override { return 0.0; }

    // Program handling
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State handling
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter access methods
    int getRate() const;
    int getDensity() const;
    int getOffset() const;
    float getGate() const;
    int getRoot() const;

    void setRate(int value);
    void setDensity(int value);
    void setOffset(int value);
    void setGate(float value);
    void setRoot(int value);

    // Custom methods
    void randomizeSequence(int patternType = 0);
    float getRateInSeconds() const;
    void generateRandomWalk();
    void generateAscendingPattern();
    void generateDescendingPattern();
    void generateArpeggioPattern();
    void setSequenceValue(int step, int value);

    // Playback control
    void setPlaying(bool shouldPlay);
    bool getIsPlaying() const;

    // Transport sync
    void setSyncToHostTransport(bool shouldSync) { syncToHostTransport = shouldSync; }

    // Public accessor methods for StepDisplay
    int getCurrentStep() const { return currentStep; }
    int getSequenceValue(int index) const { return sequence[index]; }

    // New methods for manual step control
    bool isStepEnabled(int step) const;
    void toggleStepEnabled(int step);
    void setManualStepMode(bool isManual);
    bool isManualStepMode() const { return manualStepMode; }
    void resetEnabledSteps();

    bool canAddBus(bool /*isInput*/) const override
    {
        return false;  // Don't allow adding buses
    }

    bool canRemoveBus(bool /*isInput*/) const override
    {
        return false;  // Don't allow removing buses
    }

    bool supportsDoublePrecisionProcessing() const override;
    juce::AudioProcessor::ProcessingPrecision getProcessingPrecision() const;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    bool getSyncToHostTransport() const { return syncToHostTransport; }
    double getInternalBpm() const { return internalBpm; }
    void setInternalBpm(double newBpm);
    void transposeOctaveUp();
    void transposeOctaveDown();

private:
    double internalBpm = 120.0; // Default internal BPM
    // Manual parameter values
    int rateValue;
    int densityValue;
    int offsetValue;
    float gateValue;
    int rootValue;

    // Sequencer properties
    static const int numSteps = 16;
    int currentStep = 0;
    bool isPlaying = false;
    int sequence[numSteps] = {0}; // MIDI note offsets from base note

    // New array to track enabled/disabled steps
    bool enabledSteps[numSteps] = {true}; // All steps enabled by default
    bool manualStepMode = false; // Track if we're in manual step mode

    double sampleRate = 44100.0;
    double bpm = 120.0;
    double samplesPerBeat = 0.0;
    double sampleCounter = 0.0;
    double stepDuration = 0.0;
    int stepOffset = 0;

    // Note tracking
    bool noteIsOn = false;
    int lastNoteValue = 0;

    void enhanceSequenceMelodically();

    // Transport settings
    bool syncToHostTransport = false;

    // Update timing based on host information
    void updateTimingInfo();

    // Get the active note for the current step
    int getNoteForStep(int step);

    // Calculate note length based on gate parameter
    double getNoteLength();

    // Parameter callback
    void parameterChanged(const juce::String& parameterID, float newValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomWalkSequencer)
};