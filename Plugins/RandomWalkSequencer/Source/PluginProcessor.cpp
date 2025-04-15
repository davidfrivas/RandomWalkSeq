#include "PluginProcessor.h"
#include "RandomWalkSequencer.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("MIDI In", juce::AudioChannelSet::stereo())  // Changed from disabled to stereo
                     .withOutput("MIDI Out", juce::AudioChannelSet::stereo())) // Changed from disabled to stereo
{
    // Initialize the plugin
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return "RandomWalkSequencer";
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
    return true;
}

bool AudioPluginAudioProcessor::producesMidi() const
{
    return true;
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
    return true;
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Delegate to the RandomWalkSequencer
    sequencer.prepareToPlay(sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // Delegate to the RandomWalkSequencer
    sequencer.releaseResources();
}

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

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Delegate to the RandomWalkSequencer
    sequencer.processBlock(buffer, midiMessages);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    // Create the sequencer's editor directly
    return sequencer.createEditor();
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Delegate to the RandomWalkSequencer
    sequencer.getStateInformation(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Delegate to the RandomWalkSequencer
    sequencer.setStateInformation(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}