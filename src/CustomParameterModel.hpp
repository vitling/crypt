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

struct ParameterSpec {
    String id;
    String name;
    String label;
    NormalisableRange<float> range;
    float def;
};

std::unique_ptr<AudioProcessorParameterGroup> createParameterGroup(String groupId, String groupName, std::vector<ParameterSpec> params) {
    auto group = std::make_unique<AudioProcessorParameterGroup>(groupId, groupName, "|");
    for (auto p : params) {
        group->addChild(std::make_unique<AudioParameterFloat>(ParameterID {p.id,1 }, p.name, p.range, p.def));
    }

    return std::move(group);
}