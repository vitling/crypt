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

class ParameterControlledADSR: public ADSR, public AudioProcessorValueTreeState::Listener {

    private:
    String idPrefix;
    ADSR::Parameters envParams {0.02f,0.2f,0.6f,0.5f};

    public:
    ParameterControlledADSR(String idPrefix): idPrefix(idPrefix) {
        envParams = getParameters();
    }

    static std::vector<ParameterSpec> params(String idPrefix) {
        return {
            {.id = idPrefix + "." + CryptParameters::Attack, .name = "Attack", .range = {0.0,8.0,0.001, 0.3}, .def = 0.02f},
            {.id = idPrefix + "." + CryptParameters::Decay, .name = "Decay", .range = {0.0,8.0,0.001, 0.3}, .def = 0.2f},
            {.id = idPrefix + "." + CryptParameters::Sustain, .name = "Sustain", .range = {0.0,1.0,0.01}, .def = 0.6f},
            {.id = idPrefix + "." + CryptParameters::Release, .name = "Release", .range = {0.0, 8.0, 0.001, 0.3}, .def = 0.5f},
        };
    }

    void noteOn() noexcept
    {
        setParameters(envParams);
        ADSR::noteOn();
    }

    void registerParams(AudioProcessorValueTreeState& state) {
        for (auto p: params(idPrefix)) {
            state.addParameterListener(p.id, this);
        }
    }
    void unRegisterParams(AudioProcessorValueTreeState& state) {
        for (auto p: params(idPrefix)) {
            state.removeParameterListener(p.id, this);
        }
    }
    void parameterChanged (const String& parameterID, float newValue) override {
        if (parameterID.endsWith(CryptParameters::Attack)) {
            envParams.attack = newValue;
        } else if (parameterID.endsWith(CryptParameters::Decay)) {
            envParams.decay = newValue;
        } else if (parameterID.endsWith(CryptParameters::Sustain)) {
            envParams.sustain = newValue;
        } else if (parameterID.endsWith(CryptParameters::Release)) {
            envParams.release = newValue;
        }
        // If we're currently playing then we defer applying new params until the next note
        if (!ADSR::isActive()) {
            setParameters(envParams);
        }
    }
};

