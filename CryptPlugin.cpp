/*
    Copyright 2020 David Whiting

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
    };

    /** The set of oscillators which make up this voice */
    std::vector<OscState> oscillators;

    int maxUnisonOscs = 64;
    int activeUnisonOscs = 32;

    /** Multiplier for output signal (used to scale by velocity) */
    float level = 0.0;

    /** Frequency of the currently playing note */
    float mainFrequency = 440;

    /** Reference to the parameter tree for the entire plugin so we can access parameters */
    AudioProcessorValueTreeState& state;

    /** Un-antialiased sawtooth function between 0 and TAU */
    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }

    /** Function to do the slightly awkward work of pulling a scaled (non-normalised) value from a parameter name */
    float getParameterValue(StringRef parameterName) {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

    /** Amplitude envelope for note */
    ADSR envelope;

    /** Parameters for the envelope (kept separate so we can atomically replace) */
    ADSR::Parameters envParams;

    /** Watch for changes to spread, attack and release parameters, as these need to update other elements */
    void parameterChanged(const String &parameterID, float newValue) override {
        if (parameterID == "spread") {
            setFrequency(mainFrequency, newValue, false);
        } else if (parameterID == "attack") {
            envParams.attack = newValue;
            envelope.setParameters(envParams);
        } else if (parameterID == "release") {
            envParams.release = newValue;
            envelope.setParameters(envParams);
        } else if (parameterID == "unison") {
            activeUnisonOscs = static_cast<int>(newValue);
            setFrequency(mainFrequency, getParameterValue("spread"), true);
        }
    }


    /** Clamp value between -1.0f and 1.0f */
    inline float clamp(float value) {
        return value < -1.0f ? -1.0f : value > 1.0f ? 1.0f : value;
    }

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

public:
    explicit SuperSawVoice(AudioProcessorValueTreeState& state): state(state) {
        for (int i = 0; i < maxUnisonOscs; i++) {
            oscillators.emplace_back();
        }

        state.addParameterListener("spread",this);
        state.addParameterListener("attack", this);
        state.addParameterListener("release", this);
        state.addParameterListener("unison", this);

        fillWaveTable();

        envParams = {.attack = getParameterValue("attack"), .decay = 0.1, .sustain = 1.0, .release = getParameterValue("release")};
    }

    ~SuperSawVoice() override = default;

    /**
     * Whenever we change the frequency, we need to apply the variations across all oscillators
     * @param freq Base frequency (ie. note frequency)
     * @param spread How much random deviation from the freq to apply to each oscillator
     * @param resetAngles Whether osc angles should be reset to initial positions (yes when starting new note, no when
     *                    continuing existing note
     */
    void setFrequency(float freq, float spread, bool resetAngles) {
        mainFrequency = freq;
        Random rnd;
        for (int i = 0; i < activeUnisonOscs; i++) {
            oscillators[i].frequency = freq * (1 + rnd.nextFloat() * spread - spread / 2);
            if (resetAngles) oscillators[i].angle = i / float(activeUnisonOscs) * TAU;
            oscillators[i].pan = i / float(activeUnisonOscs - 1) * 2 - 1;
            float cyclesPerSample = oscillators[i].frequency / getSampleRate();
            oscillators[i].increment = cyclesPerSample * TAU;
        }
    }

    /* We only have one sound, so this is always true */
    bool canPlaySound(juce::SynthesiserSound *) override {return true;}

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override {
        envelope.setSampleRate(getSampleRate());
        envParams = {.attack = getParameterValue("attack"), .decay = 0.1, .sustain = 1.0, .release = getParameterValue("release")};
        envelope.setParameters(envParams);

        setFrequency(MidiMessage::getMidiNoteInHertz(midiNoteNumber), getParameterValue("spread"), true);

        level = velocity * 0.04f + 0.02f;
        envelope.noteOn();
    }

    void stopNote(float velocity, bool allowTailOff) override {
        if (allowTailOff) {
            envelope.noteOff();
        } else {
            level = 0;
            envelope.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int newPitchWheelValue) override {}

    void controllerMoved(int controllerNumber, int newControllerValue) override {}

    void renderNextBlock(AudioBuffer<float> &buffer, int startSample, int numSamples) override {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        float dirt = getParameterValue("dirt");
        float shape = getParameterValue("shape");
        float tone = getParameterValue("tone") / 100.0f;

        // Save CPU if the voice is not currently playing
        if (!envelope.isActive()) {
            return;
        }

        for (auto sample = startSample; sample < startSample + numSamples; ++sample) {
            auto outL = 0.0f;
            auto outR = 0.0f;
            auto envelopeValue = envelope.getNextSample();

            for (int i = 0; i < activeUnisonOscs; i++) {
                auto &o = oscillators[i];
                float rPan = (o.pan + 1) / 2;
                float lPan = 1.0f - rPan;

                float wave = saw(o.angle);
                float shaped = clamp(wave + copysign(shape, wave));
                float toned = tone * shaped + (1.0f - tone) * wtSin(o.angle + 3 * TAU / 4);

                outL += toned * lPan;
                outR += toned * rPan;
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
};

/** This needs to exist to satisfy the needs of the Synthesiser class, but is otherwise meaningless */
class SuperSawSound : public SynthesiserSound {
public:
    SuperSawSound() = default;
    ~SuperSawSound() override = default;

    bool appliesToNote(int midiNoteNumber) override { return true; }

    bool appliesToChannel(int midiChannel) override { return true; }

};

/** The plugin itself */
class CryptAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener {
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)

    // The two main components are the Synthesiser and the Reverb (space) effect
    Synthesiser synth;
    Reverb reverb;

    const int MAX_POLYPHONY = 8;

    /** Shortcut for getting true (non-normalised) values out of a parameter tree */
    float getParameterValue(StringRef parameterName) const {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

public:

    /** Parameter state of the plugin. This is public because I am lazy. TODO: encapsulate */
    AudioProcessorValueTreeState state;

    /** Create plugin with Stereo output and setup all the parameters */
    CryptAudioProcessor() :
            AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
            state(*this, nullptr, "state", {
                    std::make_unique<AudioParameterFloat>("spread", "Spread", NormalisableRange<float>(0.0,0.1,0.001), 0.5),
                    std::make_unique<AudioParameterFloat>("shape", "Shape", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
                    std::make_unique<AudioParameterFloat>("space", "Space", NormalisableRange<float>(0.0,1.0,0.01), 0.4),
                    std::make_unique<AudioParameterFloat>("dirt", "Dirt", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
                    std::make_unique<AudioParameterFloat>("attack", "Attack", NormalisableRange<float>(0.0,8.0,0.001, 0.3), 0.2),
                    std::make_unique<AudioParameterFloat>("release", "Release", NormalisableRange<float>(0.0,8.0, 0.001, 0.3), 0.6),
                    std::make_unique<AudioParameterFloat>("tone", "Tone", NormalisableRange<float>(0.0,100.0,1.0), 100.0),
                    std::make_unique<AudioParameterFloat>("unison", "Unison", NormalisableRange<float>(4.0,64.0,1.0,0.5), 32.0),
                    std::make_unique<AudioParameterFloat>("master", "master", NormalisableRange<float>(-12.0,3.0,0.01), 0.0f)
            }) {

        // Add some voices to our empty synthesiser
        for (int i = 0; i < MAX_POLYPHONY; i++) {
            // The synth takes ownership of the voices, so this 'new' is safe
            auto voice = new SuperSawVoice(state);
            synth.addVoice(voice);
        }
        // The synth takes ownership of the Sound, so this 'new' is safe
        synth.addSound(new SuperSawSound());

        state.addParameterListener("space", this);
    }
    ~CryptAudioProcessor() override = default;

    void parameterChanged(const String &parameterID, float newValue) override {
        if (parameterID == "space") {
            setSpace(newValue);
        }
    }

    /** Whenever the 'space' parameter changes, we use it to derive a bunch of reverb settings and apply them to the
     * existing Reverb unit
     */
    void setSpace(float space) {
        Reverb::Parameters params {
                .roomSize = 0.2f + 0.8f * space,
                .damping = 0.8f - 0.7f * space,
                .wetLevel = space * 0.8f,
                .dryLevel = 1.0f - space * 0.8f,
                .width = 1.0f,
                .freezeMode = 0.0f};

        reverb.setParameters(params);
    }

    /** Before playing for the first time we need to inform components of the current sample rate, and do an inital setup
     * of the Reverb processor parameters
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        reverb.setSampleRate(sampleRate);
        setSpace(getParameterValue("space"));
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
        audio.clear();
        synth.renderNextBlock(audio, midi, 0, audio.getNumSamples());
        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
        float masterDb = getParameterValue("master");
        audio.applyGain(pow(10, masterDb/10));
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
        // note this is a total guess
        return getParameterValue("release") + 5.0 * getParameterValue("space");
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int index) override {}
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

/** GUI for the plugin */
class CryptEditor: public AudioProcessorEditor {
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CryptEditor)

    // Keep ourselves a convenient reference to the processor that made us
    CryptAudioProcessor & processor;

    /** Each parameter has a slider, label and attachement, which we group here */
    struct ParamControls {
        // unique_ptr so these get cleaned up automatically when the editor is destructed
        std::unique_ptr<Slider> slider;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
        std::unique_ptr<Label> label;
    };

    /** Mapping parameter names to their GUI control components */
    std::map<std::string, ParamControls> controls;

    /** Create a single control to control a single parameter, as specified by the paramId */
    void createControlFor(const std::string &paramId, Slider::SliderStyle style, int x, int y, int w, int h, const std::string &textBoxValueSuffix = "") {
        auto slider = std::make_unique<Slider>();
        auto attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state, paramId, *slider);
        slider->setSliderStyle(style);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 70, 20);
        slider->setTextValueSuffix(textBoxValueSuffix);
        slider->setBounds(x, y,w,h);
        addAndMakeVisible(*slider);
        controls[paramId] = {.slider = std::move(slider), .attachment = std::move(attachment)};
    }

    HyperlinkButton vitlink;
    HyperlinkButton bclink;

public:

    /** Set up the editor */
    explicit CryptEditor(CryptAudioProcessor &processor):
            AudioProcessorEditor(processor),
            processor(processor) {

        LookAndFeel &lookAndFeel = getLookAndFeel();

        lookAndFeel.setColour(Slider::ColourIds::thumbColourId, Colours::grey);
        lookAndFeel.setColour(Slider::ColourIds::trackColourId, Colours::grey.darker());
        lookAndFeel.setColour(Slider::ColourIds::backgroundColourId, Colours::grey.darker().withAlpha(0.5f));
        lookAndFeel.setColour(Slider::ColourIds::rotarySliderFillColourId, Colours::grey.darker().withAlpha(0.5f));
        lookAndFeel.setColour(Slider::ColourIds::rotarySliderOutlineColourId, Colours::grey.darker());

        setSize(500,400);
        setResizable(false, false);
        createControlFor("shape", Slider::LinearVertical, 0,100,100,200);
        createControlFor("spread", Slider::LinearVertical, 100,100,100,200);
        createControlFor("attack", Slider::RotaryHorizontalVerticalDrag, 200,100,100,100);
        createControlFor("release", Slider::RotaryHorizontalVerticalDrag, 200,200,100,100);
        createControlFor("dirt", Slider::LinearVertical, 300,100,100,200);
        createControlFor("space", Slider::LinearVertical, 400,100,100,200);
        createControlFor("tone", Slider::LinearBar, 0,350,100,30, "%");
        createControlFor("unison", Slider::LinearBar, 100,350,100,30);
        createControlFor("master", Slider::LinearBar, 400,350,100,30, "dB");

        vitlink.setURL(URL("https://www.vitling.xyz"));
        vitlink.setButtonText("Vitling");
        vitlink.setBounds(300,10,100,20);
        bclink.setURL(URL("https://bowchurch.bandcamp.com"));
        bclink.setButtonText("Bow Church");
        bclink.setBounds(400,10,100,20);

        addAndMakeVisible(vitlink);
        addAndMakeVisible(bclink);
    }

    // We use this paint function to paint our custom background image
    void paint (juce::Graphics& g) override {
        auto image = ImageCache::getFromMemory(BinaryData::ui_png, BinaryData::ui_pngSize);
        g.drawImageAt(image, 0,0);
    }
    void resized() override {

    }
};

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CryptAudioProcessor();
}

juce::AudioProcessorEditor *CryptAudioProcessor::createEditor() {
    return new CryptEditor(*this);
}