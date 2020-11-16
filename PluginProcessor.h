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
class CryptAudioProcessor  : public AudioProcessor
{
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)
    
    Synthesiser synth;
    Reverb reverb;
    
public:
    //==============================================================================
    CryptAudioProcessor() : AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
        
        for (int i = 0; i < 6; i++) {
            synth.addVoice(new SuperSawVoice(32, 0.033));
        }
        synth.addSound(new SuperSawSound());
        
        Reverb::Parameters params {
            .roomSize = 0.7f,
            .damping = 0.2f,
            .wetLevel = 0.33f,
            .dryLevel = 0.4f,
            .width = 1.0f,
            .freezeMode = 0.0f};
        
        reverb.setParameters(params);
        
    }
    ~CryptAudioProcessor() override {
        
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

    void processBlock (AudioBuffer<float>& audio, MidiBuffer& midi) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override {
        return nullptr;
    }
    bool hasEditor() const override {
        return false;
    }

    //==============================================================================
    const juce::String getName() const override {
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
