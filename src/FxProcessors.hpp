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
#pragma once

#include <JuceHeader.h>
#include "CryptParameters.hpp"
#include "CustomParameterModel.hpp"

class StereoDelay: public dsp::ProcessorBase, public AudioProcessorValueTreeState::Listener {
    private:
    dsp::DelayLine<float> delayLine;
    float feedback = 0.5;
    float wet = 0.3;
    float delayTime = 375.0f;
    float smoothedDelayTime = 0.5f;
    double sampleRate = 44100.0;

    public:

    static std::vector<ParameterSpec> params() {
        return {
            {.id = CryptParameters::DelayTime, .name = "Time", .range = {2.0,2000.0,0.1,0.5}, .def = 375.0f},
            {.id = CryptParameters::DelayMix, .name = "Mix", .range = {0.0,1.0,0.01}, .def = 0.3f},
            {.id = CryptParameters::DelayFeedback, .name = "Feedback", .range = {0.0,0.99,0.01}, .def = 0.5f}
        };
    }

    void registerParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.addParameterListener(p.id, this);
        }
    }

    void unRegisterParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.removeParameterListener(p.id, this);
        }
    }

    void parameterChanged (const String& parameterID, float newValue) override {
        if (parameterID == CryptParameters::DelayTime) {
            delayTime = newValue;
        } else if (parameterID == CryptParameters::DelayMix) {
            wet = newValue;
        } else if (parameterID == CryptParameters::DelayFeedback) {
            feedback = newValue;
        }
    }

    void prepare (const dsp::ProcessSpec &spec) override {
        delayLine.prepare(spec);
        delayLine.setMaximumDelayInSamples(spec.sampleRate * 2.1);
        sampleRate = spec.sampleRate;
    }
    void process (const dsp::ProcessContextReplacing< float > &context) override {
    
        auto input = context.getInputBlock();
        auto output = context.getOutputBlock();
        auto channels = input.getNumChannels();
        auto samples = input.getNumSamples();
        if (abs(smoothedDelayTime - delayTime) < 0.1) {
            smoothedDelayTime = delayTime;
        }
        
        for (auto i = 0; i < samples; i++) {
            smoothedDelayTime += (delayTime - smoothedDelayTime) * 0.0001;
            for (auto c = 0; c < channels; c++) {
                auto w = delayLine.popSample(c, (sampleRate / 1000) * smoothedDelayTime + smoothedDelayTime * (0.01) * c, true);
                auto d = input.getSample(c, i);
                float v = w * wet + d;
                delayLine.pushSample(c, d + feedback * w);
                output.setSample(c, i, v);
            }
        }

    }
    void reset () override {
        delayLine.reset();
    }
};

class Phaser : public dsp::ProcessorWrapper<dsp::Phaser<float>>, public AudioProcessorValueTreeState::Listener {
    public:
    Phaser() {
        processor.setCentreFrequency(1000.0f);
        processor.setDepth(0.5f);
        processor.setRate(0.2f);
        processor.setMix(0.15f);
    }
    static std::vector<ParameterSpec> params() {
        return {
            {.id = CryptParameters::PhaserDepth, .name = "Depth", .range = {0.0,1.0,0.01}, .def = 0.5f},
            {.id = CryptParameters::PhaserRate, .name = "Rate", .range = {0.02,1.0,0.01,0.5}, .def = 0.2f},
            {.id = CryptParameters::PhaserMix, .name = "Mix", .range = {0.0,1.0,0.01}, .def = 0.3f}
        };
    }

    void registerParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.addParameterListener(p.id, this);
        }
    }

    void unRegisterParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.removeParameterListener(p.id, this);
        }
    }

    void parameterChanged (const String& parameterID, float newValue) override {
        if (parameterID == CryptParameters::PhaserDepth) {
            processor.setDepth(newValue);
        } else if (parameterID == CryptParameters::PhaserRate) {
            processor.setRate(newValue);
        } else if (parameterID == CryptParameters::PhaserMix) {
            processor.setMix(newValue * 0.5f); // 0.5 is actually full "mix" because it's half phased and half normal signal
        }
    }
    
};

class CryptReverb : public dsp::ProcessorWrapper<dsp::Reverb>, public AudioProcessorValueTreeState::Listener {
    private:
    void setSpace(float space) {
        Reverb::Parameters params {
                .roomSize = 0.2f + 0.8f * space,
                .damping = 0.8f - 0.7f * space,
                .wetLevel = space * 0.8f,
                .dryLevel = 1.0f - space * 0.8f,
                .width = 1.0f,
                .freezeMode = 0.0f};

        processor.setParameters(params);
    }
    void parameterChanged (const String& parameterID, float newValue) override {
        if (parameterID == CryptParameters::Space) {
            setSpace(newValue);
        } 
    }

    public:
    static std::vector<ParameterSpec> params() {
        return {
            {.id = CryptParameters::Space, .name = "Space", .range = {0.0,1.0,0.01}, .def = 0.2f},
        };
    }

    void registerParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.addParameterListener(p.id, this);
        }
    }

    void unRegisterParams(AudioProcessorValueTreeState& state) {
        for (auto p: params()) {
            state.removeParameterListener(p.id, this);
        }
    }

    CryptReverb() {
        setSpace(0.2f);
    }
};
