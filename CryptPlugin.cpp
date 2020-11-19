//
// Created by David Whiting on 2020-11-19.
//

#include <JuceHeader.h>

#define TAU MathConstants<float>::twoPi

struct OscState {
    float angle = 0.0;
    float frequency = 440;
    float increment = 0.0;
    float pan = 0.0;
};

class SuperSawVoice : public SynthesiserVoice, public AudioProcessorValueTreeState::Listener {
private:
    std::vector<OscState> oscs;
    int maxUnisonVoices = 32;
    int activeUnisonVoices = 32;

    float level = 0.0;
    float mainFrequency = 440;

    AudioProcessorValueTreeState& state;

    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }

    float getParameterValue(StringRef parameterName) {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

    ADSR envelope;
    ADSR::Parameters envParams;

    void parameterChanged(const String &parameterID, float newValue) override {
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

public:
    explicit SuperSawVoice(AudioProcessorValueTreeState& state): state(state) {
        for (int i = 0; i < maxUnisonVoices; i++) {
            oscs.emplace_back();
        }

        state.addParameterListener("spread",this);
        state.addParameterListener("attack", this);
        state.addParameterListener("release", this);
        envParams = {.attack = getParameterValue("attack"), .decay = 0.1, .sustain = 1.0, .release = getParameterValue("release")};
    }

    ~SuperSawVoice() override = default;

    void setFrequency(float freq, float spread, bool resetAngles) {
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
};

class SuperSawSound : public SynthesiserSound {
public:
    SuperSawSound() = default;
    ~SuperSawSound() override = default;

    bool appliesToNote(int midiNoteNumber) override { return true; }

    bool appliesToChannel(int midiChannel) override { return true; }

};

class CryptAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CryptAudioProcessor)

    Synthesiser synth;
    Reverb reverb;

    float getParameterValue(StringRef parameterName) const {
        auto param = state.getParameter(parameterName);
        return param->convertFrom0to1(param->getValue());
    }

public:

    AudioProcessorValueTreeState state;

    //==============================================================================
    CryptAudioProcessor() :
            AudioProcessor(BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
            state(*this, nullptr, "state", {
                    std::make_unique<AudioParameterFloat>("spread", "Spread", NormalisableRange<float>(0.0,0.1,0.001), 0.3),
                    std::make_unique<AudioParameterFloat>("shape", "Shape", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
                    std::make_unique<AudioParameterFloat>("space", "Space", NormalisableRange<float>(0.0,1.0,0.01), 0.4),
                    std::make_unique<AudioParameterFloat>("dirt", "Dirt", NormalisableRange<float>(0.0,1.0,0.01), 0.0),
                    std::make_unique<AudioParameterFloat>("attack", "Attack", NormalisableRange<float>(0.0,6.0,0.001, 0.3), 0.0),
                    std::make_unique<AudioParameterFloat>("release", "Release", NormalisableRange<float>(0.0,5.0, 0.001, 0.3), 0.0)
            }) {


        for (int i = 0; i < 8; i++) {
            auto voice = new SuperSawVoice(state);
            synth.addVoice(voice);
        }
        synth.addSound(new SuperSawSound());

        state.addParameterListener("space", this);
    }
    ~CryptAudioProcessor() override = default;

    void parameterChanged(const String &parameterID, float newValue) override {
        if (parameterID == "space") {
            setSpace(newValue);
        }
    }

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

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        reverb.setSampleRate(sampleRate);
        setSpace(getParameterValue("space"));
    }
    void releaseResources() override {

    }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return (layouts.getMainOutputChannels() == 2);
    }

    void processBlock (AudioBuffer<float>& audio, MidiBuffer& midi) override {
        audio.clear();
        synth.renderNextBlock(audio, midi, 0,audio.getNumSamples());
        reverb.processStereo(audio.getWritePointer(0), audio.getWritePointer(1), audio.getNumSamples());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {
        return true;
    }

    //==============================================================================
    const String getName() const override {
        return "Crypt";
    }

    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 0.0;}

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) override { return "Basement"; }
    void changeProgramName (int index, const String& newName) override { }

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override {
        auto stateToSave = state.copyState();
        std::unique_ptr<XmlElement> xml (stateToSave.createXml());
        copyXmlToBinary(*xml, destData);
    }
    void setStateInformation (const void* data, int sizeInBytes) override {
        std::unique_ptr<XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
        if (xmlState != nullptr) {
            if (xmlState->hasTagName(state.state.getType())) {
                state.replaceState(ValueTree::fromXml(*xmlState));
            }
        }
    }

};


class CryptEditor: public AudioProcessorEditor {
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CryptEditor)

    CryptAudioProcessor & processor;

    struct ParamControls {
        std::unique_ptr<Slider> slider;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
        std::unique_ptr<Label> label;
    };

    std::map<std::string, ParamControls> controls;

public:

    void createControlFor(const std::string &paramId, Slider::SliderStyle style, int x, int y, int w, int h) {
        auto slider = std::make_unique<Slider>();
        auto attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state, paramId, *slider);
        slider->setSliderStyle(style);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 70, 20);
        slider->setBounds(x, y,w,h);
        addAndMakeVisible(*slider);
        controls[paramId] = {.slider = std::move(slider), .attachment = std::move(attachment)};
    }

    explicit CryptEditor(CryptAudioProcessor &processor):
            AudioProcessorEditor(processor),
            processor(processor) {

        setSize(500,400);
        setResizable(false, false);
        createControlFor("shape", Slider::LinearVertical, 0,100,100,200);
        createControlFor("spread", Slider::LinearVertical, 100,100,100,200);
        createControlFor("attack", Slider::RotaryHorizontalVerticalDrag, 200,100,100,100);
        createControlFor("release", Slider::RotaryHorizontalVerticalDrag, 200,200,100,100);
        createControlFor("dirt", Slider::LinearVertical, 300,100,100,200);
        createControlFor("space", Slider::LinearVertical, 400,100,100,200);

    }

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