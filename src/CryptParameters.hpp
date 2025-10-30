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

#define P(X) const String X = #X;

namespace CryptParameters {
    const String Unison = "Unison";
    const String Spread = "Spread";
    const String Shape = "Shape";
    const String Dirt = "Dirt";
    /// + ADSR

    const String Cutoff = "Cutoff";
    const String Resonance = "Resonance";
    const String FilterEnv = "FilterEnv";
    /// + ADSR

    const String DelayTime = "DelayTime";
    const String DelayMix = "DelayMix";
    const String DelayFeedback = "DelayFeedback";

    const String PhaserDepth = "PhaserDepth";
    const String PhaserRate = "PhaserRate";
    const String PhaserMix = "PhaserMix";

    const String Space = "Space";

    const String Attack = "Attack";
    const String Decay = "Decay";
    const String Sustain = "Sustain";
    const String Release = "Release";

    const String PitchBendRange = "PitchBendRange";
    const String Master = "Master";

    // ID prefixes
    const String Amplitude = "Amplitude";
    const String Filter = "Filter";
    

    std::map<String, String> unitMap {
        {Cutoff, "Hz"},
        {DelayTime, "ms"},
        {PhaserRate, "Hz"},
        {Attack, "s"},
        {Decay, "s"},
        {Release, "s"},
        {Master, "dB"},
        {PitchBendRange, " st"}
    };

    std::map<String, String> labelMap {
        {DelayTime, "Time"},
        {DelayMix, "Mix"},
        {DelayFeedback, "Feedback"},
        {PhaserDepth, "Depth"},
        {PhaserRate, "Rate"},
        {PhaserMix, "Mix"},
        {FilterEnv, "Env Amount"},
        {PitchBendRange, "PB Range"}
    };

    StringRef getUnit(StringRef param) {
        auto out = unitMap.find(param);
        if (out != unitMap.end()) {
            return out->second;
        } else {
            return "";
        }
    }

    StringRef getLabel(StringRef paramId) {
        auto out = labelMap.find(paramId);
        if (out != labelMap.end()) {
            return out->second;
        } else {
            return paramId;
        }
    }
}

#undef P