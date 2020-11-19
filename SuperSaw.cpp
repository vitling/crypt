/*
  ==============================================================================

    SuperSaw.cpp
    Created: 14 Nov 2020 3:48:28pm
    Author:  David Whiting

  ==============================================================================
*/

#include "SuperSaw.h"

SuperSawVoice::SuperSawVoice(AudioProcessorValueTreeState &state) : state(state) {
    for (int i = 0; i < maxUnisonVoices; i++) {
        oscs.emplace_back();
    }

    state.addParameterListener("spread",this);
    state.addParameterListener("attack", this);
    state.addParameterListener("release", this);
    envParams = {.attack = getParameterValue("attack"), .decay = 0.1, .sustain = 1.0, .release = getParameterValue("release")};
}

inline float clamp(float value) {
    return value < -1.0f ? -1.0f : value > 1.0f ? 1.0f : value;
}

inline float shapeCompoundWave(float f, float dirt) {
    constexpr float factor = 2.0f;
    f = f * (1.0f/factor + dirt*10.0f);
    if (f < -1.0f) {
        return factor * -2/3.0f;
    } else if (f > 1.0f) {
        return factor * 2/3.0f;
    } else {
        return factor * (f - (f * f * f)/3.0f);
    }
}

void SuperSawVoice::renderNextBlock(AudioBuffer<float> &buffer, int startSample, int numSamples) {
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    float dirt = getParameterValue("dirt");
    float shape = getParameterValue("shape");

    if (!envelope.isActive()) {
        return;
    }


    for (auto sample = startSample; sample < startSample + numSamples; ++sample) {
        auto outL = 0.0f;
        auto outR = 0.0f;
        auto envelopeValue = envelope.getNextSample();

        for (int i = 0; i < activeUnisonVoices; i++) {
            auto &o = oscs[i];
            float rPan = (o.pan + 1) / 2;
            float lPan = 1.0f - rPan;

            float wave = saw(o.angle);
            float shaped = clamp(wave + copysign(shape, wave));

            outL += shaped * lPan;
            outR += shaped * rPan;
            o.angle += o.increment;
            if (o.angle > TAU) o.angle -= TAU;
        }

        left[sample] += shapeCompoundWave(outL, dirt) * level * envelopeValue;
        right[sample] += shapeCompoundWave(outR, dirt) * level * envelopeValue;
    }

    if (!envelope.isActive()) {
        clearCurrentNote();
    }

}

void SuperSawVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound,
                              int currentPitchWheelPosition) {
    envelope.setSampleRate(getSampleRate());
    envParams = {.attack = getParameterValue("attack"), .decay = 0.1, .sustain = 1.0, .release = getParameterValue("release")};
    envelope.setParameters(envParams);
    setFrequency(MidiMessage::getMidiNoteInHertz(midiNoteNumber), getParameterValue("spread"), true);
    level = velocity * 0.04f + 0.02f;
    envelope.noteOn();
}

void SuperSawVoice::stopNote(float velocity, bool allowTailOff) {
    if (allowTailOff) {
        envelope.noteOff();
    } else {
        level = 0;
        envelope.reset();
        clearCurrentNote();
    }
}

void SuperSawVoice::setFrequency(float freq, float spread, bool resetAngles) {
    mainFrequency = freq;
    Random rnd;
    for (int i = 0; i < activeUnisonVoices; i++) {
        oscs[i].frequency = freq * (1 + rnd.nextFloat() * spread - spread/2);
        if (resetAngles) oscs[i].angle = i / float(activeUnisonVoices) * TAU;
        oscs[i].pan = i / float(activeUnisonVoices) * 2 - 1;
        float cyclesPerSample = oscs[i].frequency / getSampleRate();
        oscs[i].increment = cyclesPerSample * TAU;
    }
}

void SuperSawVoice::parameterChanged(const String &parameterID, float newValue) {
    if (parameterID == "spread") {
        setFrequency(mainFrequency, newValue, false);
    } else if (parameterID == "attack") {
        envParams.attack = newValue;
        envelope.setParameters(envParams);
    } else if (parameterID == "release") {
        envParams.release = newValue;
        envelope.setParameters(envParams);
    }
}


