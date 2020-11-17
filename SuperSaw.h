/*
  ==============================================================================

    SuperSaw.h
    Created: 14 Nov 2020 3:48:28pm
    Author:  David Whiting

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define PI MathConstants<float>::pi
#define TAU MathConstants<float>::twoPi


struct OscState {
    float angle = 0.0;
    float frequency = 440;
    float increment = 0.0;
    float pan = 0.0;
};

class SuperSawVoice : public SynthesiserVoice, public AudioProcessorValueTreeState::Listener {
private:
    std::vector<OscState> oscs;
    int maxUnisonVoices = 32;
    int activeUnisonVoices = 32;
    
    float level = 0.0;
    float mainFrequency = 440;

    AudioProcessorValueTreeState& state;
    
    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }

    float getParameterValue(StringRef parameterName) {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }
    
    ADSR envelope;
    ADSR::Parameters envParams;

    void parameterChanged(const String &parameterID, float newValue) override;

public:
    explicit SuperSawVoice(AudioProcessorValueTreeState& state);
    
    ~SuperSawVoice() override = default;

    void setFrequency(float freq, bool resetAngles);
    
    bool canPlaySound(juce::SynthesiserSound *) override {return true;}
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
    
    void stopNote(float velocity, bool allowTailOff) override;
    
    void pitchWheelMoved(int newPitchWheelValue) override {}
    
    void controllerMoved(int controllerNumber, int newControllerValue) override {}
    
    void renderNextBlock(AudioBuffer<float> &buffer, int startSample, int numSamples) override;
};

class SuperSawSound : public SynthesiserSound {
public:
    SuperSawSound() = default;
    ~SuperSawSound() override = default;
    
    bool appliesToNote(int midiNoteNumber) override { return true; }
    
    bool appliesToChannel(int midiChannel) override { return true; }

};

class SuperSawSynthesiser : public Synthesiser {

};