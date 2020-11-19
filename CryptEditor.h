//
// Created by David Whiting on 2020-11-16.
//

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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
