#include "PluginProcessor.h"


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CryptAudioProcessor();
}

void CryptAudioProcessor::processBlock(AudioBuffer<float>& audio, MidiBuffer& midi) {
    audio.clear();
    synth.renderNextBlock(audio, midi, 0,audio.getNumSamples());
    reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
}
