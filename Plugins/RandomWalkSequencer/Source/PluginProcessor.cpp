#include "PluginProcessor.h"
#include "RandomWalkSequencer.h"

//==============================================================================
/**
 * Constructor for the main audio plugin processor
 * Sets up the plugin with stereo MIDI input and output buses
 */
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("MIDI In", juce::AudioChannelSet::stereo())  // Changed from disabled to stereo
                     .withOutput("MIDI Out", juce::AudioChannelSet::stereo())) // Changed from disabled to stereo
{
    // Initialize the plugin
}

/**
 * Destructor - cleanup resources if needed
 */
AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

//==============================================================================
/**
 * Returns the name of the plugin that will be displayed in the host
 */
const juce::String AudioPluginAudioProcessor::getName() const
{
    return "RandomWalkSequencer";
}

/**
 * Indicates that the plugin accepts MIDI input
 */
bool AudioPluginAudioProcessor::acceptsMidi() const
{
    return true;
}

/**
 * Indicates that the plugin outputs MIDI messages
 */
bool AudioPluginAudioProcessor::producesMidi() const
{
    return true;
}

/**
 * Indicates this is a MIDI effect rather than an audio processor
 */
bool AudioPluginAudioProcessor::isMidiEffect() const
{
    return true;
}

/**
 * Returns the tail length in seconds - zero for this MIDI processor
 */
double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

/**
 * Returns the number of stored configurations (presets)
 */
int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

/**
 * Returns the index of the current preset
 */
int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

/**
 * Sets the current preset to the specified index
 */
void AudioPluginAudioProcessor::setCurrentProgram(int index) {}

/**
 * Returns the name of the specified preset
 */
const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    return {};
}

/**
 * Assigns a new name to the specified preset
 */
void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

//==============================================================================
/**
 * Initializes the processor before playback starts
 * Delegates initialization to the underlying RandomWalkSequencer
 */
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Delegate to the RandomWalkSequencer
    sequencer.prepareToPlay(sampleRate, samplesPerBlock);
}

/**
 * Releases resources when the plugin is deactivated
 * Delegates cleanup to the underlying RandomWalkSequencer
 */
void AudioPluginAudioProcessor::releaseResources()
{
    // Delegate to the RandomWalkSequencer
    sequencer.releaseResources();
}

/**
 * Validates if the specified bus layout is supported by this plugin
 * Must match the RandomWalkSequencer's implementation for consistency
 */
bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // This function must match the RandomWalkSequencer's implementation
    bool isInputDisabled = layouts.getMainInputChannelSet().isDisabled();
    bool isOutputDisabled = layouts.getMainOutputChannelSet().isDisabled();

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

/**
 * Processes an audio/MIDI block
 * Delegates all processing to the RandomWalkSequencer
 */
void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Delegate to the RandomWalkSequencer
    sequencer.processBlock(buffer, midiMessages);
}

//==============================================================================
/**
 * Indicates that this plugin has a custom editor UI
 */
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

/**
 * Creates the plugin's custom editor UI
 * Directly uses the sequencer's editor
 */
juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    // Create the sequencer's editor directly
    return sequencer.createEditor();
}

//==============================================================================
/**
 * Saves the plugin's current state to memory
 * Delegates state saving to the RandomWalkSequencer
 */
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Delegate to the RandomWalkSequencer
    sequencer.getStateInformation(destData);
}

/**
 * Restores the plugin state from previously saved data
 * Delegates state restoration to the RandomWalkSequencer
 */
void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Delegate to the RandomWalkSequencer
    sequencer.setStateInformation(data, sizeInBytes);
}

//==============================================================================
/**
 * Factory function that creates new instances of the plugin processor
 * Called by the host when loading the plugin
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}