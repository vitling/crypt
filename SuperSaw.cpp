/*
  ==============================================================================

    SuperSaw.cpp
    Created: 14 Nov 2020 3:48:28pm
    Author:  David Whiting

  ==============================================================================
*/

#include "SuperSaw.h"

void SuperSawVoice::renderNextBlock(AudioBuffer<float> &buffer, int startSample, int numSamples) {
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    if (!envelope.isActive()) {
        return;
    }


    for (auto sample = startSample; sample < startSample + numSamples; ++sample) {
        auto outL = 0.0;
        auto outR = 0.0;
        auto envelopeValue = envelope.getNextSample();

        for (int i = 0; i < activeUnisonVoices; i++) {
            auto &o = oscs[i];
            float rPan = (o.pan + 1) / 2;
            float lPan = 1.0f - rPan;

            outL += saw(o.angle) * lPan;
            outR += saw(o.angle) * rPan;
            o.angle += o.increment;
            //o.angle -= o.angle > TAU ? TAU : 0;
            if (o.angle > TAU) o.angle -= TAU;
        }

        left[sample] += outL * level * envelopeValue;
        right[sample] += outR * level * envelopeValue;
    }

    if (!envelope.isActive()) {
        clearCurrentNote();
    }

}

void SuperSawVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound,
                              int currentPitchWheelPosition) {
    envelope.setSampleRate(getSampleRate());
    envelope.setParameters(envParams);
    setFrequency(MidiMessage::getMidiNoteInHertz(midiNoteNumber), true);
    level = velocity * 0.04f;
    tailOff = 0;
    envelope.noteOn();
}

void SuperSawVoice::stopNote(float velocity, bool allowTailOff) {
    if (allowTailOff) {
        envelope.noteOff();
        tailOff = 1.0;
    } else {
        level = 0;
        envelope.reset();
        clearCurrentNote();
    }
}

void SuperSawVoice::setFrequency(float freq, bool resetAngles) {
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
