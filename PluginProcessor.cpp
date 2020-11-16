#include "PluginProcessor.h"


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CryptAudioProcessor();
}

