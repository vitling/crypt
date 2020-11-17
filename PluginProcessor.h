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

    float dirtValue = 0.0f;

public:

    AudioParameterFloat* spread;
    AudioParameterFloat* space;
    AudioParameterFloat* shape;
    AudioParameterFloat* dirt;

    //==============================================================================
    CryptAudioProcessor() : AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
        addParameter(spread = new AudioParameterFloat("Spread", "Spread", {0, 0.06, 0.002}, 0.03));
        addParameter(space = new AudioParameterFloat("Space", "Space", {0,1,0.01}, 0.4));
        addParameter(shape = new AudioParameterFloat("Shape", "Shape", {0, 1, 0.01}, 0.0f));
        addParameter(dirt = new AudioParameterFloat("Dirt", "Dirt", {0, 1, 0.01}, 0.0f));
        spread->addListener(this);
        space->addListener(this);
        shape->addListener(this);
        dirt->addListener(this);

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
        } else if (parameterIndex == shape->getParameterIndex()) {
            for (int i =0 ; i < synth.getNumVoices(); i++) {
                auto *voice = dynamic_cast<SuperSawVoice*>(synth.getVoice(i));
                voice->setShaper(shape->get());
            }
        } else if (parameterIndex == dirt->getParameterIndex()) {
            dirtValue = dirt->get();
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

    float waveShape(float f) {
        f = f * (1.0 + dirtValue * 10);
        if (f < -1.0f) {
            return -2/3.0f;
        } else if (f > 1.0f) {
            return 2/3.0f;
        } else {
            return f - (f * f * f)/3.0f;
        }
    }

    void processBlock (AudioBuffer<float>& audio, MidiBuffer& midi) override {
//        IIRFilter filter;
//        filter.setCoefficients(IIRCoefficients::makeLowPass(getSampleRate(), 3000.0, 1));

        audio.clear();
        synth.renderNextBlock(audio, midi, 0,audio.getNumSamples());
        auto *l = audio.getWritePointer(0);
        auto *r = audio.getWritePointer(1);

//        filter.processSamples(l, audio.getNumSamples());
//        filter.processSamples(r, audio.getNumSamples());

        for (int i =0 ; i < audio.getNumSamples(); i++) {
            l[i] = waveShape(l[i] * 10)/10;
            r[i] = waveShape(r[i] * 10)/10;
        }

        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
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
