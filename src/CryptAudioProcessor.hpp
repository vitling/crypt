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
#include "ParameterControlledADSR.hpp"
#include "SuperSawVoice.hpp"
#include "SharedBuffer.hpp"
#include "FxProcessors.hpp"

/** This needs to exist to satisfy the needs of the Synthesiser class, but is otherwise meaningless */
class AlwaysOnSound : public SynthesiserSound {
public:
    AlwaysOnSound() = default;
    ~AlwaysOnSound() override = default;

    bool appliesToNote(int midiNoteNumber) override { return true; }

    bool appliesToChannel(int midiChannel) override { return true; }

};

/*  The simple presets in Crypt are managed by a baked-in XML file, containing a set of possible plugin
    states in the same form as they are saved by the createXml function of an AudioProcessorValueTreeState.
    This baked-in XML is a BinaryData resource in resources/presets.xml */
struct Preset {
    int id;
    String name;
    std::unique_ptr<XmlElement> stateData;
};

class PresetManager {
    typedef std::pair<String, std::unique_ptr<XmlElement>> NamedPreset;

    private:
    std::vector<Preset> presetData;

    void loadPresets() {
        DBG("Loading Preset");
        String content (BinaryData::presets_xml);
        auto parsed = parseXML(content);

        if (parsed) {
            DBG("Document parsed successfully");
            if (parsed->hasTagName("presets")) {
                DBG("Presets as top level tag, good!");
                int n = 1;
                for (auto preset: parsed->getChildIterator()) {
                    auto name = preset->getChildByName("name")->getAllSubText();
                    DBG("Loading preset " << name);
                    auto element = std::make_unique<XmlElement>(*(preset->getChildByName("state")));
                    presetData.push_back({n++, name, std::move(element)});
                    DBG("successfully inserted into map");
                }
            }
        } else {
            DBG("Failed XML Parse");
        }
    }
    public:

    PresetManager() {
        loadPresets();
    }

    void applyPreset(int index, AudioProcessorValueTreeState &state) {
        state.replaceState(ValueTree::fromXml(*presetData[index-1].stateData.get()));
    }

    std::vector<StringRef> listPresets() {
        std::vector<StringRef> result(presetData.size());
        std::transform(presetData.begin(), presetData.end(), result.begin(), [](Preset& x) { return x.name; });
        return result;
    }
};


/** The plugin itself */
class CryptAudioProcessor  : public AudioProcessor {
    friend class CryptAudioProcessorEditor;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)

    Synthesiser synth;

    dsp::ProcessorChain<Phaser, CryptReverb, StereoDelay> fxRig;

    SharedBuffer oscBuffer;

    MidiKeyboardState keyboardState;

    // This was kind of an arbitrary choice
    const int MAX_POLYPHONY = 8;

    /* Shortcut for getting true (non-normalised) values out of a parameter tree 
     * I honestly cannot remember why I'm not using getRawParameterValue, but I remember crashes
     * when I tried to rationalise all the parameter stuff and I'm scared to change it now
     */
    float getParameterValue(StringRef parameterName) const {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

    static AudioProcessorValueTreeState::ParameterLayout createCryptParameterLayout() {
        std::vector<std::unique_ptr<AudioProcessorParameterGroup>> params;

        auto phaser =     createParameterGroup("Phaser", "Phaser", Phaser::params());
        auto delay =      createParameterGroup("Delay", "Delay", StereoDelay::params());
        auto oscillator = createParameterGroup("Osc", "Oscillator", SuperSawVoice::params());
        auto reverb =     createParameterGroup("Reverb", "Reverb", CryptReverb::params());
        auto ampEnv =     createParameterGroup("Amplitude", "Amp Env",
                            ParameterControlledADSR::params(CryptParameters::Amplitude));
        auto filterEnv =  createParameterGroup("Filter", "Filter Env", 
                            ParameterControlledADSR::params(CryptParameters::Filter));
        
        return {
            std::move(oscillator),
            std::move(phaser),
            std::move(delay), 
            std::move(reverb),
            std::move(ampEnv),
            std::move(filterEnv),
            std::make_unique<AudioParameterFloat>(
                ParameterID {CryptParameters::Master, 1},
                "Master Gain",
                NormalisableRange<float>(-12.0,3.0,0.01),
                0.0f)
        };
    }

public:

    AudioProcessorValueTreeState state;

    PresetManager presetManager;

    /** Create plugin with Stereo output and setup all the parameters */
    CryptAudioProcessor() :
            AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
            state(*this, nullptr, "state", createCryptParameterLayout()),
            oscBuffer(512) {

        // Add some voices to our empty synthesiser
        for (int i = 0; i < MAX_POLYPHONY; i++) {
            // The synth takes ownership of the voices, so this 'new' is safe
            auto voice = new SuperSawVoice(state);
            synth.addVoice(voice);
        }
        // The synth takes ownership of the Sound, so this 'new' is safe
        synth.addSound(new AlwaysOnSound());
        fxRig.get<0>().registerParams(state);
        fxRig.get<1>().registerParams(state);
        fxRig.get<2>().registerParams(state);
    }
    ~CryptAudioProcessor() override {
        fxRig.get<0>().unRegisterParams(state);
        fxRig.get<1>().unRegisterParams(state);
        fxRig.get<2>().unRegisterParams(state);
    }

    /** Before playing for the first time we need to inform components of the current sample rate, and do an inital setup
     * of the Reverb processor parameters
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        fxRig.prepare({.sampleRate = sampleRate, .maximumBlockSize = (uint32)samplesPerBlock, .numChannels = 2});
    }

    /** Everything we've allocated will be self-destructed, so there's no resources to release */
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return (layouts.getMainOutputChannels() == 2);
    }

    /** Main audio generating segment. There is nothing in the chain that requires creating extra buffers, so this same
     * AudioBuffer is passed around everywhere and only ever incremented
     */
    void processBlock (AudioBuffer<float>& audio, MidiBuffer& midi) override {
        keyboardState.processNextMidiBuffer(midi, 0, audio.getNumSamples(), true);

        audio.clear();
        synth.renderNextBlock(audio, midi, 0, audio.getNumSamples());
        
        dsp::AudioBlock<float> block(audio);
        dsp::ProcessContextReplacing<float> context(block);

        fxRig.process(context);

        float masterDb = getParameterValue(CryptParameters::Master);
        audio.applyGain(pow(10, masterDb/10));

        // Buffer for waveform visualisation
        oscBuffer.write(audio.getNumSamples(), audio.getReadPointer(0));
    }

    // We need to defer this implementation until the end of the file, when we have defined our editor
    juce::AudioProcessorEditor* createEditor() override;

    // Various metadata about the plugin
    bool hasEditor() const override { return true; }
    const String getName() const override { return "Crypt";}
    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {
        // note this is a total guess, probably should have
        // done a real calculation
        return 5.0;
    }

    
    // Made an executive decision to just not support this type of program switching
    // behaviour and instead only expose a small selection of presets from the UI
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int index) override { }
    const String getProgramName (int index) override { return "Default Program"; }
    void changeProgramName (int index, const String& newName) override { }

    // Save state to binary block (used eg. for saving state inside Ableton project)
    void getStateInformation (MemoryBlock& destData) override {
        auto stateToSave = state.copyState();
        std::unique_ptr<XmlElement> xml (stateToSave.createXml());
        copyXmlToBinary(*xml, destData);
    }

    // Restore state from binary block
    void setStateInformation (const void* data, int sizeInBytes) override {
        std::unique_ptr<XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
        
        if (xmlState != nullptr) {
            if (xmlState->hasTagName(state.state.getType())) {
                state.replaceState(ValueTree::fromXml(*xmlState));
            }
        }
    }
};