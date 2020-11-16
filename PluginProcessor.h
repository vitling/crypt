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
class CryptAudioProcessor  : public AudioProcessor , AudioProcessorParameter::Listener
{
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)

    Synthesiser synth;
    Reverb reverb;

    AudioParameterFloat* spread;
    AudioParameterFloat* space;
    
public:
    //==============================================================================
    CryptAudioProcessor() : AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
        addParameter(spread = new AudioParameterFloat("Spread", "Spread", {0, 0.06, 0.002}, 0.03));
        addParameter(space = new AudioParameterFloat("Space", "Space", {0,1,0.01}, 0.4));

        spread->addListener(this);
        space->addListener(this);

        for (int i = 0; i < 6; i++) {
            synth.addVoice(new SuperSawVoice());
        }
        synth.addSound(new SuperSawSound());

        setSpace(space->get());
    }
    ~CryptAudioProcessor() override = default;


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

    void parameterValueChanged(int parameterIndex, float newValue) override {
        if (parameterIndex == spread->getParameterIndex()) {
            for (int i =0 ; i < synth.getNumVoices(); i++) {
                auto *voice = dynamic_cast<SuperSawVoice*>(synth.getVoice(i));
                voice->setSpread(spread->get());
            }
        } else if (parameterIndex == space->getParameterIndex()) {
            setSpace(space->get());
        }
    }

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {

    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        reverb.setSampleRate(sampleRate);
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
    juce::AudioProcessorEditor* createEditor() override {
        return nullptr;
    }
    bool hasEditor() const override {
        return false;
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
    void getStateInformation (MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

};
