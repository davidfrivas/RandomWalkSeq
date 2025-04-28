#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "RandomWalkSequencer.h" // Include the sequencer

//==============================================================================
/**
 * Main audio processor class for the RandomWalkSequencer plugin
 * Acts as a wrapper that delegates all functionality to the RandomWalkSequencer class
 */
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    /**
     * Constructor - initializes the plugin with appropriate MIDI buses
     */
    AudioPluginAudioProcessor();

    /**
     * Destructor - cleans up resources if needed
     */
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    /**
     * Prepares the processor for playback by setting sample rate and buffer size
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    /**
     * Releases resources when the processor is no longer needed
     */
    void releaseResources() override;

    /**
     * Checks if the provided bus layout is compatible with this processor
     */
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    /**
     * Processes incoming audio/MIDI data and produces output
     */
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    /**
     * Creates the editor component for the plugin UI
     */
    juce::AudioProcessorEditor* createEditor() override;

    /**
     * Indicates whether this processor has a custom editor
     */
    bool hasEditor() const override;

    //==============================================================================
    /**
     * Returns the name of the plugin
     */
    const juce::String getName() const override;

    /**
     * Indicates if the processor can receive MIDI input
     */
    bool acceptsMidi() const override;

    /**
     * Indicates if the processor outputs MIDI messages
     */
    bool producesMidi() const override;

    /**
     * Indicates if this is a MIDI effect (rather than an audio processor)
     */
    bool isMidiEffect() const override;

    /**
     * Returns the tail time of the processor in seconds
     */
    double getTailLengthSeconds() const override;

    //==============================================================================
    /**
     * Returns the number of programs (presets) available
     */
    int getNumPrograms() override;

    /**
     * Returns the current program (preset) index
     */
    int getCurrentProgram() override;

    /**
     * Sets the current program (preset) to the specified index
     */
    void setCurrentProgram (int index) override;

    /**
     * Gets the name of the specified program (preset)
     */
    const juce::String getProgramName (int index) override;

    /**
     * Changes the name of the specified program (preset)
     */
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    /**
     * Saves the current state of the processor to the provided memory block
     */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /**
     * Restores the processor state from the provided data
     */
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    // The RandomWalkSequencer that handles the actual MIDI generation
    RandomWalkSequencer sequencer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};