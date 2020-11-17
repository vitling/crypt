/*
  ==============================================================================

    SuperSaw.h
    Created: 14 Nov 2020 3:48:28pm
    Author:  David Whiting

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define PI MathConstants<double>::pi
#define TAU MathConstants<double>::twoPi


struct OscState {
    float angle = 0.0;
    float frequency = 440;
    float increment = 0.0;
    float pan = 0.0;
};

class SuperSawVoice : public SynthesiserVoice {
private:
    std::vector<OscState> oscs;
    int maxUnisonVoices = 32;
    int activeUnisonVoices = 32;
    float spread = 0.03;
    
    float level = 0.0;
    float tailOff = 0.0;
    float mainFrequency = 440;
    float shaper = 0.0;
    
    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }
    
    ADSR envelope;
    ADSR::Parameters envParams;

public:
    SuperSawVoice() {
        for (int i = 0; i < maxUnisonVoices; i++) {
            oscs.emplace_back();
        }
        envParams = {.attack = 0.1, .decay = 0.1, .sustain = 0.7, .release = 0.7};
    
    }
    
    ~SuperSawVoice() override = default;

    void setFrequency(float freq, bool resetAngles);
    
    bool canPlaySound(juce::SynthesiserSound *) override {
        return true;
    }

    void setSpread(float newValue) {
        spread = newValue;
        setFrequency(mainFrequency, false);
    }

    void setShaper(float newValue) {
        shaper = newValue;
    }
    
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