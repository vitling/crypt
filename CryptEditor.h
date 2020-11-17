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

    explicit CryptEditor(CryptAudioProcessor &processor):
    AudioProcessorEditor(processor),
    processor(processor) {

        setSize(420,250);
        setResizable(false, false);
        std::vector<std::string> params = {"spread","shape","dirt","attack","release","space"};
        int xPosition = 0;
        for (auto &param: params) {
            auto slider = std::make_unique<Slider>();
            auto label = std::make_unique<Label>();
            auto attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state, param, *slider);
            slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 70, 20);
            slider->setBounds(xPosition, 50,70,120);
            label->attachToComponent(&*slider,false);
            label->setText(processor.state.getParameter(param)->getName(20), dontSendNotification);
            //label->setSize(50,50);
            addAndMakeVisible(*slider);
            addAndMakeVisible(*label);
            controls[param] = {.slider = std::move(slider), .attachment = std::move(attachment), .label = std::move(label)};
            xPosition += 70;
        }

    }

    void paint (juce::Graphics& g) override {
        //g.fillAll(Colours::black);
    }
    void resized() override {

    }

};
