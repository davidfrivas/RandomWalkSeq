#include "PluginProcessor.h"
#include "RandomWalkSequencer.h"

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RandomWalkSequencer();
}
