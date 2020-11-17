//
// Created by David Whiting on 2020-11-16.
//

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CryptEditor: public AudioProcessorEditor, Slider::Listener {
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CryptEditor)

    Slider spread;
    Slider space;
    CryptAudioProcessor & processor;
public:
    explicit CryptEditor(CryptAudioProcessor &processor): AudioProcessorEditor(processor), processor(processor) {
        setSize(300,200);
        setResizable(false, false);
        spread.setSliderStyle(Slider::LinearVertical);
        spread.setRange(0,1,0.01);
        spread.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);
        spread.setValue(0.03);
        spread.setBounds(0,0,50,200);
        spread.addListener(this);
        addAndMakeVisible(spread);
    }
    void sliderValueChanged(Slider *slider) override {
        if (slider == &spread) {
            processor.spread->setValueNotifyingHost(slider->getValue());
        }
    }

    void paint (juce::Graphics& g) override {
        g.fillAll(Colours::black);
    }
    void resized() override {

    }

};
