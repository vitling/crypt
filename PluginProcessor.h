/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "SuperSaw.h"


//==============================================================================
/**
*/
class CryptAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)

    Synthesiser synth;
    Reverb reverb;

    float getParameterValue(StringRef parameterName) const {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

public:

    AudioProcessorValueTreeState state;

    //==============================================================================
    CryptAudioProcessor() :
        AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        state(*this, nullptr, "state", {
            std::make_unique<AudioParameterFloat>("spread", "Spread", NormalisableRange<float>(0.0,0.1,0.001), 0.3),
            std::make_unique<AudioParameterFloat>("shape", "Shape", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
            std::make_unique<AudioParameterFloat>("space", "Space", NormalisableRange<float>(0.0,1.0,0.01), 0.4),
            std::make_unique<AudioParameterFloat>("dirt", "Dirt", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
            std::make_unique<AudioParameterFloat>("attack", "Attack", NormalisableRange<float>(0.0,6.0,0.001, 0.3), 0.0),
            std::make_unique<AudioParameterFloat>("release", "Release", NormalisableRange<float>(0.0,5.0, 0.001, 0.3), 0.0)
        }) {


        for (int i = 0; i < 8; i++) {
            auto voice = new SuperSawVoice(state);
            synth.addVoice(voice);
        }
        synth.addSound(new SuperSawSound());

        state.addParameterListener("space", this);
    }
    ~CryptAudioProcessor() override = default;

    void parameterChanged(const String &parameterID, float newValue) override {
        if (parameterID == "space") {
            setSpace(newValue);
        }
    }

    void setSpace(float space) {
        Reverb::Parameters params {
            .roomSize = 0.2f + 0.8f * space,
            .damping = 0.8f - 0.7f * space,
            .wetLevel = space * 0.8f,
            .dryLevel = 1.0f - space * 0.8f,
            .width = 1.0f,
            .freezeMode = 0.0f};

        reverb.setParameters(params);
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        reverb.setSampleRate(sampleRate);
        setSpace(getParameterValue("space"));
    }
    void releaseResources() override {
        
    }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return (layouts.getMainOutputChannels() == 2);
    }

    void processBlock (AudioBuffer<float>& audio, MidiBuffer& midi) override {
        audio.clear();
        synth.renderNextBlock(audio, midi, 0,audio.getNumSamples());
        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {
        return true;
    }

    //==============================================================================
    const String getName() const override {
        return "Crypt";
    }

    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 0.0;}

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) override { return "Basement"; }
    void changeProgramName (int index, const String& newName) override { }

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override {
        auto stateToSave = state.copyState();
        std::unique_ptr<XmlElement> xml (stateToSave.createXml());
        copyXmlToBinary(*xml, destData);
    }
    void setStateInformation (const void* data, int sizeInBytes) override {
        std::unique_ptr<XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
        if (xmlState != nullptr) {
            if (xmlState->hasTagName(state.state.getType())) {
                state.replaceState(ValueTree::fromXml(*xmlState));
            }
        }
    }

};
