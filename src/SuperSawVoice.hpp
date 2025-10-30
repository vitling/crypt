/*
    Copyright 2025 David Whiting

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <JuceHeader.h>
#include "CryptParameters.hpp"
#include "CustomParameterModel.hpp"
#include "ParameterControlledADSR.hpp"

#define TAU MathConstants<float>::twoPi

/**
 * A single voice of the polyphonic synthesiser
 */
class SuperSawVoice : public SynthesiserVoice, public AudioProcessorValueTreeState::Listener {
private:
    /** Stores the state of a single oscillator (of many) within the Voice */
    struct OscState {
        float angle = 0.0;
        float frequency = 440;
        float increment = 0.0;
        float pan = 0.0;
        float spreadRnd = 0.0;
    };

    /** The set of oscillators which make up this voice */
    std::vector<OscState> oscillators;

    int maxUnisonOscs = 64;
    int activeUnisonOscs = 32;

    /** Multiplier for output signal (used to scale by velocity) */
    float level = 0.0;

    float mainFrequency = 440;

    float pitchBend = 0.0;

    int pitchBendRange = 2;

    int midiNote = 4;

    float shape = 0.0f;
    float dirt = 0.0f;
    float cutoff = 20000.0f;
    float resonance = 1.0f;
    float filterEnv = 0.0f;
    float spread = 0.03f;

    /** Reference to the parameter tree for the entire plugin so we can access parameters */
    AudioProcessorValueTreeState& state;

    /** Un-antialiased sawtooth function between 0 and TAU */
    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }

    static inline float square(float angle) {
        return std::copysign(1.0f, angle - MathConstants<float>::pi);
    }

    ParameterControlledADSR ampEnvelope { CryptParameters::Amplitude };
    ParameterControlledADSR filterEnvelope { CryptParameters::Filter };

    dsp::StateVariableTPTFilter<float> filter;

    /** Waveshaping function which applies a cubic clipping curve, gained by the 'dirt' parameter */
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

    static constexpr int SINE_WAVETABLE_SIZE = 512;
    float sinTable[SINE_WAVETABLE_SIZE];

    void fillWaveTable() {
        for (auto i = 0; i < SINE_WAVETABLE_SIZE; i++) {
            float angle = TAU * i / SINE_WAVETABLE_SIZE;
            sinTable[i] = sin(angle);
        }
    }

    float wtSin(float angle) {
        jassert(angle >= 0.0f);

        return sinTable[static_cast<int>(SINE_WAVETABLE_SIZE * angle / TAU) % SINE_WAVETABLE_SIZE];
    }

    void parameterChanged(const String &parameterID, float newValue) override {
        if (parameterID == CryptParameters::Spread) {
            spread = newValue;
            setFrequency(mainFrequency, spread, false);
        } else if (parameterID == CryptParameters::Unison) {
            activeUnisonOscs = static_cast<int>(newValue);
            setFrequency(mainFrequency, spread, true);
        } else if (parameterID == CryptParameters::Shape) {
            shape = newValue;
        } else if (parameterID == CryptParameters::Dirt) {
            dirt = newValue;
        } else if (parameterID == CryptParameters::Cutoff) {
            cutoff = newValue;
        } else if (parameterID == CryptParameters::Resonance) {
            resonance = newValue;
        } else if (parameterID == CryptParameters::FilterEnv) {
            filterEnv = newValue;
        } else if (parameterID == CryptParameters::PitchBendRange) {
            pitchBendRange = newValue;
        }
    }

    

public:

    static std::vector<ParameterSpec> params() {
        std::vector<ParameterSpec> params = {
            {.id = CryptParameters::Unison, .name = "Unison Voices", .range = {4.0,64.0,1.0,0.5}, .def = 32.0f},
            {.id = CryptParameters::Spread, .name = "Unison Spread", .range = {0.0, 0.1, 0.001}, .def = 0.03f},
            {.id = CryptParameters::Shape, .name = "Osc Shape", .range = {0.0, 1.0, 0.01}, .def = 0.0f},
            {.id = CryptParameters::Dirt, .name = "Dirt", .range = {0.0,1.0,0.01}, .def = 0.0f},
            {.id = CryptParameters::Cutoff, .name = "Filter Cutoff", .range = {50.0,20000.0,1.0,0.2}, .def = 20000.0f},
            {.id = CryptParameters::Resonance, .name = "Filter Resonance", .range = {0.1,6.0,0.01}, .def = 1.0f},
            {.id = CryptParameters::FilterEnv, .name = "Filter Env Amount", .range = {0.0,1.0,0.001}, .def = 0.0f},
            {.id = CryptParameters::PitchBendRange, .name = "Pitchbend Range", .range = {0,12,1}, .def = 2.0f},
        };
        return params;
    }

    void registerParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.addParameterListener(p.id, this);
        }
        ampEnvelope.registerParams(state);
        filterEnvelope.registerParams(state);
    }

    void unRegisterParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.removeParameterListener(p.id, this);
        }
        ampEnvelope.unRegisterParams(state);
        filterEnvelope.unRegisterParams(state);
    }

    explicit SuperSawVoice(AudioProcessorValueTreeState& state): state(state) {
        for (int i = 0; i < maxUnisonOscs; i++) {
            oscillators.emplace_back();
        }
        fillWaveTable();
        registerParams(state);
        // We need to prepare with something before we hit a note, because we may hit renderBlock before startNote
        filter.prepare({ .sampleRate = 44100, .maximumBlockSize = 8192, .numChannels = 2 });
    }

    /**
     * Whenever we change the frequency, we need to apply the variations across all oscillators
     * @param freq Base frequency (ie. note frequency)
     * @param spread How much random deviation from the freq to apply to each oscillator
     * @param resetAngles Whether osc angles should be reset to initial positions (yes when starting new note, no when
     *                    continuing existing note
     */
    void setFrequency(float freq, float spread, bool phaseReset) {
        mainFrequency = freq;
        Random rnd;
        for (int i = 0; i < activeUnisonOscs; i++) {
            if (phaseReset) {
                oscillators[i].angle = i / float(activeUnisonOscs) * TAU;
                oscillators[i].spreadRnd = rnd.nextFloat();
            }
            oscillators[i].frequency = freq * (1 + oscillators[i].spreadRnd * spread - spread / 2);
            oscillators[i].pan = i / float(activeUnisonOscs - 1) * 2 - 1;
            float cyclesPerSample = oscillators[i].frequency / getSampleRate();
            oscillators[i].increment = cyclesPerSample * TAU;
        }
    }

    float calcFrequency(int midiNoteNumber, int pitchWheelValue) {
        const float pitchBend = (float(pitchWheelValue) / 8192.0)-1.0;
        return MidiMessage::getMidiNoteInHertz(midiNoteNumber) * pow(2, pitchBend * pitchBendRange / 12.0);
    }

    /* We only have one sound, so this is always true */
    bool canPlaySound(juce::SynthesiserSound *) override {return true;}

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override {
        ampEnvelope.reset();
        ampEnvelope.setSampleRate(getSampleRate());
        filterEnvelope.reset();
        filterEnvelope.setSampleRate(getSampleRate());
    
        midiNote = midiNoteNumber;
        setFrequency(calcFrequency(midiNoteNumber, currentPitchWheelPosition), spread, true);

        filter.prepare({ .sampleRate = getSampleRate(), .maximumBlockSize = 8192, .numChannels = 2});

        level = velocity * 0.04f + 0.02f;
        ampEnvelope.noteOn();
        filterEnvelope.noteOn();
    }

    void pitchWheelMoved (int newPitchWheelValue) override {
        
        setFrequency(calcFrequency(midiNote, newPitchWheelValue), spread, false);
    }

    void stopNote(float velocity, bool allowTailOff) override {
        if (allowTailOff) {
            ampEnvelope.noteOff();
            filterEnvelope.noteOff();

        } else {
            level = 0;
            ampEnvelope.reset();
            filterEnvelope.reset();
            clearCurrentNote();
        }
    }

    void controllerMoved(int controllerNumber, int newControllerValue) override {}

    void renderNextBlock(AudioBuffer<float> &buffer, int startSample, int numSamples) override {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        // Approximated function to reduce volume as number of oscs increases. I didn't do the actual
        // maths here to figure out what the function should be , just went for a function that gave a
        // pleasing response curve to it.
        float unisonScaleFactor = 3.0f / sqrt(4.0f + (float)activeUnisonOscs);

        filter.setCutoffFrequency(cutoff);
        filter.setResonance(resonance);

        // Save CPU if the voice is not currently playing
        if (!ampEnvelope.isActive()) {
            clearCurrentNote();
            return;
        }

        for (auto sample = startSample; sample < startSample + numSamples; ++sample) {
            auto outL = 0.0f;
            auto outR = 0.0f;
            auto envelopeValue = ampEnvelope.getNextSample();
            auto filterEnvValue = filterEnvelope.getNextSample();

            for (int i = 0; i < activeUnisonOscs; i++) {
                auto &o = oscillators[i];
                float rPan = (o.pan + 1) / 2;
                float lPan = 1.0f - rPan;

                float wave = saw(o.angle);
                float shaped = std::clamp(wave + copysign(shape, wave), -1.0f, 1.0f);

                outL += unisonScaleFactor * shaped * lPan;
                outR += unisonScaleFactor * shaped * rPan;
                o.angle += o.increment;
                if (o.angle > TAU) o.angle -= TAU;
            }

            float cutoffWithEnv = cutoff * pow(2.0f, (filterEnv * 4.0f * filterEnvValue));
            filter.setCutoffFrequency(cutoffWithEnv > 20000.0f ? 20000.0f : cutoffWithEnv);
            
            left[sample] += shapeCompoundWave(filter.processSample(0, outL), dirt) * level * envelopeValue;
            right[sample] += shapeCompoundWave(filter.processSample(1, outR), dirt) * level * envelopeValue;
        }

        if (!ampEnvelope.isActive()) {
            clearCurrentNote();
        }

    }
};
