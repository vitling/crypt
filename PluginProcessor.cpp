#include "PluginProcessor.h"

#include "CryptEditor.h"

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CryptAudioProcessor();
}

juce::AudioProcessorEditor *CryptAudioProcessor::createEditor() {
    return new CryptEditor(*this);
}
