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
#include "CryptAudioProcessor.hpp"

const Colour CRYPT_BLUE = Colour::fromString("ff60a5ca");

class EmbeddedFonts {
private:
    Font gothicaBook;

public:
    EmbeddedFonts() {
        // Experimentally disabled, to see if it helps Linux users
        // gothicaBook = Font(Typeface::createSystemTypefaceFor(BinaryData::GothicaBook_ttf, BinaryData::GothicaBook_ttfSize));
    }
    const Font& getGothicaBook() const {
        return gothicaBook;
    }
};

EmbeddedFonts fonts;

class CryptLookAndFeel: public LookAndFeel_V4 {
    public:
    CryptLookAndFeel() {
        auto thumb = CRYPT_BLUE;
        this->setColour(Slider::ColourIds::thumbColourId, thumb);
        this->setColour(Slider::ColourIds::trackColourId, Colours::orange);
        this->setColour(Slider::ColourIds::backgroundColourId, Colours::black);
        this->setColour(Slider::ColourIds::rotarySliderFillColourId, thumb);
        this->setColour(Slider::ColourIds::rotarySliderOutlineColourId, Colours::black);
        this->setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::transparentBlack);
        this->setColour(Label::outlineColourId, Colours::transparentBlack);
        this->setColour(HyperlinkButton::ColourIds::textColourId, CRYPT_BLUE);

        this->setColour(MidiKeyboardComponent::ColourIds::blackNoteColourId, CRYPT_BLUE.darker(0.6f));
        this->setColour(MidiKeyboardComponent::ColourIds::whiteNoteColourId, Colours::black);
        this->setColour(MidiKeyboardComponent::ColourIds::keySeparatorLineColourId, CRYPT_BLUE);
        this->setColour(MidiKeyboardComponent::ColourIds::mouseOverKeyOverlayColourId, CRYPT_BLUE);
        this->setColour(MidiKeyboardComponent::ColourIds::keyDownOverlayColourId, CRYPT_BLUE);
        this->setColour(MidiKeyboardComponent::ColourIds::textLabelColourId, CRYPT_BLUE);
        this->setColour(MidiKeyboardComponent::ColourIds::shadowColourId, Colours::transparentWhite);

        this->setDefaultSansSerifTypeface(fonts.getGothicaBook().getTypefacePtr());
        
    }

    // After numerous attempts to remove the box from the value label with conventional methods I just gave up and overrid it here
    void drawLabel (Graphics& g, Label& label) override {
        g.fillAll (label.findColour (Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font (getLabelFont (label));

            g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
            g.setFont (font);

            auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                            jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                            label.getMinimumHorizontalScale());
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (Label::outlineColourId));
        }
    }

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override {
        auto outline = slider.findColour (Slider::rotarySliderOutlineColourId);
        auto fill    = slider.findColour (Slider::rotarySliderFillColourId);

        auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (3);

        auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = jmin (8.0f, radius * 0.2f);
        auto arcRadius = radius - lineW * 0.2f;

        Path backgroundArc;
        backgroundArc.addCentredArc (bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    rotaryEndAngle,
                                    true);

        g.setColour (outline);
        g.strokePath (backgroundArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    toAngle,
                                    true);

            g.setColour (fill);
            g.strokePath (valueArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
        }

        auto thumbWidth = lineW * 2.0f;
        Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - MathConstants<float>::halfPi),
                                bounds.getCentreY() + arcRadius * std::sin (toAngle - MathConstants<float>::halfPi));

        g.setColour (slider.findColour (Slider::thumbColourId));
        g.drawLine(bounds.getCentreX(), bounds.getCentreY(), thumbPoint.getX(), thumbPoint.getY(),2);
    }

};

class LabelledDial: public Component {
    private:
    AudioProcessorValueTreeState &state;
    Slider slider;
    Label label;
    AudioProcessorValueTreeState::SliderAttachment attachment;

    public:
    LabelledDial(AudioProcessorValueTreeState &state, StringRef parameterId, StringRef labelText = "", StringRef suffix = ""):
        state(state),
        slider(),
        attachment(state, parameterId, slider)
     {
        
        slider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        if (suffix.isNotEmpty()) {
            slider.setTextValueSuffix(suffix);
        
        }
        
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);

        if (labelText.isEmpty()) {
            label.setText(CryptParameters::getLabel(parameterId),NotificationType::dontSendNotification);
        } else {
            label.setText(labelText,NotificationType::dontSendNotification);
        }
        label.setJustificationType(Justification::centred);
        label.setFont(fonts.getGothicaBook().withHeight(20));
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
    }

    void resized() override {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromTop(20));
        slider.setBounds(area);
    }
};

class ControlGroup: public GroupComponent {
private:
    String title;
    AudioProcessorValueTreeState &state;
    std::vector<std::unique_ptr<LabelledDial>> controls;
    Component* visualiser;

public:

    ControlGroup(StringRef title, AudioProcessorValueTreeState &state, std::list<StringRef> parameters, Component* visualiser = nullptr): title(title), state(state), visualiser(visualiser) {
        for (auto param: parameters) {
            auto x = std::make_unique<LabelledDial>(state, param, CryptParameters::getLabel(param), CryptParameters::getUnit(param));
            addAndMakeVisible(*x);
            controls.push_back(std::move(x));
        }
        if (visualiser) {
            addAndMakeVisible(*visualiser);
        }
        
        setText(title);
    }

    void resized() override {
        auto contentsBounds = getLocalBounds().reduced(20);
        
        int width = contentsBounds.getWidth();
        int height = 80;

        int nControls = controls.size();

        auto controlBounds = contentsBounds.removeFromTop(height);
        if (visualiser) {
            visualiser->setBounds(contentsBounds);
        }
    
        float controlWidth = width / nControls;
        for (int i = 0 ; i < nControls; i++) {
            auto &c = controls[i];
            c->setBounds({controlBounds.getX() + i * (int)controlWidth, controlBounds.getY(), (int)controlWidth, height});
        }
    }
    
};

class ADSREditor: public GroupComponent {

    class Viewer: public Component, public AudioProcessorValueTreeState::Listener, private AsyncUpdater {
        private:
        AudioProcessorValueTreeState &state;
        String prefix;
        String attackParam, decayParam, sustainParam, releaseParam;

        void handleAsyncUpdate() override {
            repaint();
        }

        public:
        Viewer(String prefix, AudioProcessorValueTreeState &state)
            :   prefix(prefix),
                state(state),
                attackParam(prefix + CryptParameters::Attack),
                decayParam(prefix + CryptParameters::Decay),
                sustainParam(prefix + CryptParameters::Sustain),
                releaseParam(prefix + CryptParameters::Release) {

            state.addParameterListener(attackParam, this);
            state.addParameterListener(decayParam, this);
            state.addParameterListener(sustainParam, this);
            state.addParameterListener(releaseParam, this);
        }

        ~Viewer() {
            state.removeParameterListener(attackParam, this);
            state.removeParameterListener(decayParam, this);
            state.removeParameterListener(sustainParam, this);
            state.removeParameterListener(releaseParam, this);
        }

        void parameterChanged (const String& parameterID, float newValue) override {
            triggerAsyncUpdate();
        }

        void paint (juce::Graphics& graphics) override {
            auto width = getWidth();
            auto height = getHeight();

            graphics.fillAll(Colours::black);

            float attack = *state.getRawParameterValue(attackParam);
            float decay =  *state.getRawParameterValue(decayParam);
            float sustain = *state.getRawParameterValue(sustainParam);
            float release = *state.getRawParameterValue(releaseParam);

            auto totalTime = attack + decay + release + 1.0;
            float xScale = (double) width / totalTime;
            float yScale = height-10;

            graphics.setColour(Colours::darkgrey.withAlpha(0.7f));
            auto step = totalTime < 2.0 ? 0.1 : totalTime < 8.0 ? 0.5 : 1.0;
            for (auto x = 0.0; x < totalTime; x += step) {
                if (x < attack + decay || x > attack + decay + 1.0) {
                    graphics.fillRect(x * xScale, 0, 1, height);
                }
            }

            graphics.setColour(CRYPT_BLUE);

            
            Point<float>    start(0, yScale+5), 
                            peak(xScale * attack, 5),
                            sus1(xScale * (attack + decay), yScale * (1-sustain)+5),
                            sus2(xScale * (attack + decay + 1.0), yScale * (1-sustain)+5),
                            end(width, yScale+5);

            float curve = 0;
            Path path;
            path.startNewSubPath({0.0f, yScale+5});
            
            path.cubicTo(start.translated(curve,0), peak.translated(-curve, 0), peak);
            path.cubicTo(peak.translated(curve,0), sus1.translated(-curve,0), sus1);
            path.cubicTo(sus1.translated(curve,0), sus2.translated(-curve,0), sus2);

            path.cubicTo(sus2.translated(curve,0), end.translated(-curve,0), end);
            

            PathStrokeType s(2);

            graphics.strokePath(path, s);

        }
    };

    private:
    LabelledDial a, d, s, r;
    Viewer viewer;

    public:
    ADSREditor(AudioProcessorValueTreeState& state, String prefix, StringRef title):
            a(state, prefix + CryptParameters::Attack, "A", CryptParameters::getUnit(CryptParameters::Attack)),
            d(state, prefix + CryptParameters::Decay, "D", CryptParameters::getUnit(CryptParameters::Decay)),
            s(state, prefix + CryptParameters::Sustain, "S", CryptParameters::getUnit(CryptParameters::Sustain)),
            r(state, prefix + CryptParameters::Release, "R", CryptParameters::getUnit(CryptParameters::Release)),
            viewer(prefix, state) {

        addAndMakeVisible(a);
        addAndMakeVisible(d);
        addAndMakeVisible(s);
        addAndMakeVisible(r);

        addAndMakeVisible(viewer);
        setText(title);
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(15);
        auto adsr = bounds.removeFromTop(80);
        auto controlWidth = adsr.getWidth() / 4;
        a.setBounds(adsr.removeFromLeft(controlWidth));
        d.setBounds(adsr.removeFromLeft(controlWidth));
        s.setBounds(adsr.removeFromLeft(controlWidth));
        r.setBounds(adsr.removeFromLeft(controlWidth));
        viewer.setBounds(bounds);
    }

};


class DelayDisplay: public Component, public AudioProcessorValueTreeState::Listener, private AsyncUpdater {
    private:
    float delayTime = 100.0f;
    float delayMix = 0.3f;
    float delayFeedback = 0.5;

    AudioProcessorValueTreeState &state;

    void handleAsyncUpdate() override
    {
        repaint();
    }

    public:
    DelayDisplay(AudioProcessorValueTreeState &state): state(state) {
        state.addParameterListener(CryptParameters::DelayTime, this);
        state.addParameterListener(CryptParameters::DelayFeedback, this);
        state.addParameterListener(CryptParameters::DelayMix, this);
        delayTime = *state.getRawParameterValue(CryptParameters::DelayTime);
        delayFeedback =*state.getRawParameterValue(CryptParameters::DelayFeedback);
        delayMix = *state.getRawParameterValue(CryptParameters::DelayMix);
    }

    ~DelayDisplay() {
        state.removeParameterListener(CryptParameters::DelayTime, this);
        state.removeParameterListener(CryptParameters::DelayFeedback, this);
        state.removeParameterListener(CryptParameters::DelayMix, this);
    }

    void parameterChanged (const String& parameterID, float newValue) override {
        if (parameterID == CryptParameters::DelayTime) {
            delayTime = newValue;
        } else if (parameterID == CryptParameters::DelayFeedback) {
            delayFeedback = newValue;
        } else if (parameterID == CryptParameters::DelayMix) {
            delayMix = newValue;
        } 
        triggerAsyncUpdate();
    }
    void paint(Graphics &g) override {

        Rectangle<int> bounds = getLocalBounds().reduced(10);

        // we're thinking in ms
        float xResolution = 4000;
        g.setColour(CRYPT_BLUE);
        g.fillRect(bounds.getX(), bounds.getY(), 5, bounds.getHeight());
        float amount = 1.0f;
        float x = 0;
        g.setColour(CRYPT_BLUE.withAlpha(delayMix * 0.7f + 0.3f));
        while (amount > 0.01 && x < xResolution) {
            x += delayTime;
            auto h = amount * bounds.getHeight();
            g.fillRect(bounds.getX() + (x / xResolution) * (float)bounds.getWidth(), (float)bounds.getCentreY() - h/2, 5.0f, h);
            amount *= delayFeedback;
        }


    }
};

// Oscillator visualiser in the oscillator section
class OscDisplay: public Component, public AudioProcessorValueTreeState::Listener, private AsyncUpdater {
    private:
    AudioProcessorValueTreeState &state;

    static inline float saw(float angle) {
        return (2.0f * angle/TAU) - 1;
    }

    static inline float square(float angle) {
        return  angle < (TAU / 2) ? -1.0f : 1.0f;
    }

    static inline float cycle(float angle) {
        return angle - static_cast<int>(angle / TAU) * TAU;
    }

    void handleAsyncUpdate() override
    {
        repaint();
    }

    public:
    OscDisplay(AudioProcessorValueTreeState &state): state(state) {
        state.addParameterListener(CryptParameters::Shape, this);
        state.addParameterListener(CryptParameters::Unison, this);
        state.addParameterListener(CryptParameters::Spread, this);
    }

    ~OscDisplay() {
        state.removeParameterListener(CryptParameters::Shape, this);
        state.removeParameterListener(CryptParameters::Unison, this);
        state.removeParameterListener(CryptParameters::Spread, this);
    }

    void parameterChanged (const String& parameterID, float newValue) override {

        triggerAsyncUpdate();
    }
    void paint(Graphics &g) override {

        float shape = *state.getRawParameterValue(CryptParameters::Shape);
        int unison = static_cast<int>(*state.getRawParameterValue(CryptParameters::Unison));
        float spread = *state.getRawParameterValue(CryptParameters::Spread);

        DBG("shape = " << shape << " unison = " << unison << " spread = " << spread);

        auto bounds = getLocalBounds();
        Path p;
        p.startNewSubPath(0,bounds.getHeight()/2);
        for (auto xp = 0; xp < bounds.getWidth(); xp++) {
            auto x = jmap<float>((float)xp/bounds.getWidth(), 0, TAU * 8);
            auto angle = cycle(x);
            auto y = - shape * square(angle) - (1.0f - shape) * saw(angle);
            auto yp = jmap<float>((y + 1) / 2, bounds.getHeight() * 0.2, bounds.getHeight() * 0.8);
            p.lineTo(xp, yp);
        }

        g.setColour(CRYPT_BLUE.withAlpha(0.5f));
        for (auto i = 0 ; i < unison; i++) {
            float vSpread = (((float)i / unison)* 2.0 - 1.0) * spread * 10;
            float distance = vSpread * 30;

            auto transform = AffineTransform::translation(-getWidth()/2.0, 0).scaled((4.0 + vSpread)/4.0, 1.0).translated(getWidth()/2.0, distance);

            g.strokePath(p, PathStrokeType(1), transform);
        }
    }
};

// Waveform display running down the middle
class WaveformDisplay: public Component, Timer {
private:
    DrawableImage i;
    Image img;
    SharedBuffer & buffer;
    public:

    WaveformDisplay(SharedBuffer & buffer): buffer(buffer), img(Image::PixelFormat::ARGB, 200,500,true) {
        i.setImage(img);
        
        addAndMakeVisible(i);
        startTimerHz(30);
    }
    

    void resized() override {
        i.setBounds(getLocalBounds());
        i.setTransformToFit(getLocalBounds().toFloat(), RectanglePlacement::stretchToFit);
    }

    void timerCallback() override {
        update();
    }

    float taperFunction(float x) {
        return x < 0.5f ? cos((x - 0.5f) * TAU)/2 + 0.5f : 1.0f;
    }

    void update() {
        img.clear(img.getBounds(), Colours::transparentBlack);
        Graphics g(i.getImage());
        
        buffer.read();
        auto & displayBuffer = buffer.get();
        
        auto area = g.getClipBounds();
        
        Path waveformPath;
        waveformPath.startNewSubPath(area.getCentreX(), 0);

        float max = 0.01f;
        for (size_t i = 0; i < displayBuffer.size(); ++i) {
            if (abs(displayBuffer[i]) > max) {
                max = abs(displayBuffer[i]);
            }
        }
        
        float scaleFactor = 0.7f / max;
        float dbSize = displayBuffer.size();

        for (size_t i = 0; i < dbSize; ++i)
        {
            auto value = displayBuffer[i] * scaleFactor * taperFunction((float)i / dbSize);
            auto x = jmap<float>(value, -1.0f, 1.0f, area.getRight(), area.getX());
            auto y = jmap<float>(i, 0, displayBuffer.size(), 0, area.getHeight());
            waveformPath.lineTo(x, y);
        }
        g.setColour (CRYPT_BLUE.withAlpha(0.5f));
        g.strokePath(waveformPath, PathStrokeType(6.0f));

        g.setColour (Colours::white);
        g.strokePath(waveformPath, PathStrokeType(2.0f));
        
        const MessageManagerLock mml;
        if (mml.lockWasGained()) {
            repaint();
        }
    }
};

class KeyboardToggleButton: public Button {

    private:
    Image keyboardIcon;
    bool isEnabled;

    protected:
    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
        auto mainColor = CRYPT_BLUE;
        
        g.setColour(CRYPT_BLUE.withAlpha((isEnabled ? 0.8f : 0.4f) + (shouldDrawButtonAsHighlighted ? 0.1f : 0.0f) + (shouldDrawButtonAsDown ? 0.1f : 0.0f)));

        g.drawImage(keyboardIcon, getLocalBounds().toFloat().reduced(10), RectanglePlacement::stretchToFit, true);
    }

    public:
    KeyboardToggleButton(): Button("Key"), keyboardIcon(ImageCache::getFromMemory(BinaryData::keyboardicon_png, BinaryData::keyboardicon_pngSize)) {
    }

    void setEnabled(bool enabled) {
        isEnabled = enabled;
        repaint();
    }


};

class CryptKeyboardComponent: public MidiKeyboardComponent {
    public:

    CryptKeyboardComponent(MidiKeyboardState &state): MidiKeyboardComponent(state, MidiKeyboardComponent::horizontalKeyboard) {}

    void drawBlackNote (int /*midiNoteNumber*/, Graphics& g, Rectangle<float> area,
                                            bool isDown, bool isOver, Colour noteFillColour) override {
        auto c = noteFillColour;

        if (isDown)  c = c.overlaidWith (findColour (keyDownOverlayColourId));
        if (isOver)  c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

        g.setColour (c);
        g.fillRect (area);

        if (isDown)
        {
            g.setColour (noteFillColour);
            g.drawRect (area);
        }
        else
        {
            // This used to be where the shadow got drawn, but I overrid this method to remove it to
            // give myself a flat keyboard
        }
    }
};


/** GUI for the plugin */
class CryptAudioProcessorEditor: public AudioProcessorEditor, public Button::Listener, public ComboBox::Listener {
private:
    // Must come first so it's destroyed last
    CryptLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CryptAudioProcessorEditor)

    CryptAudioProcessor & processor;

    WaveformDisplay visualiser;

    CryptKeyboardComponent keyboard;
    OscDisplay oscDisplay;
    DelayDisplay delayDisplay;

    ADSREditor ampEnv, filterEnv;
    ControlGroup osc, filter, phaser, delay, theVoid, global;
    Label pluginTitle;

    ComboBox presets;
    TextButton save;
    TextButton load;

    HyperlinkButton bowchurch;
    HyperlinkButton vitling;
    HyperlinkButton donate;

    TooltipWindow tooltipWindow;

    KeyboardToggleButton keyboardButton;
    bool keyboardIsVisible = false;
    const int keyboardHeight = 75;

    Image keyboardButtonImage;

    std::unique_ptr<FileChooser> fileChooser;

    Label versionNumber;

public:

    /** Set up the editor */
    explicit CryptAudioProcessorEditor(CryptAudioProcessor &processor):
            AudioProcessorEditor(processor),
            processor(processor),
            keyboard(processor.keyboardState),
            visualiser(processor.oscBuffer),
            ampEnv(processor.state, CryptParameters::Amplitude + ".", "Amp Env"),
            filterEnv(processor.state, CryptParameters::Filter + ".", "Filter Env"),
            oscDisplay(processor.state),
            delayDisplay(processor.state),
            osc("Oscillator", processor.state, {CryptParameters::Unison, CryptParameters::Spread, CryptParameters::Shape}, &oscDisplay),
            filter("Filter", processor.state, {CryptParameters::Cutoff, CryptParameters::Resonance, CryptParameters::FilterEnv}),
            phaser("Phaser", processor.state, {CryptParameters::PhaserDepth, CryptParameters::PhaserRate, CryptParameters::PhaserMix}),
            delay("Delay", processor.state, {CryptParameters::DelayTime, CryptParameters::DelayFeedback, CryptParameters::DelayMix}, &delayDisplay),
            theVoid("Void", processor.state, {CryptParameters::Dirt, CryptParameters::Space}),
            global("Globals", processor.state, {CryptParameters::PitchBendRange, CryptParameters::Master}),
            tooltipWindow(this) {

        setLookAndFeel(&lookAndFeel);

        pluginTitle.setText("CRYPT",NotificationType::dontSendNotification);
        pluginTitle.setFont(fonts.getGothicaBook().withHeight(40));
        pluginTitle.setJustificationType(Justification::horizontallyCentred);

        setSize(1000,625);
        setResizable(true, true);

        setResizeLimits(800,625,1000,625);

        addAndMakeVisible(ampEnv);
        addAndMakeVisible(filterEnv);
        addAndMakeVisible(osc);
        addAndMakeVisible(filter);
        addAndMakeVisible(phaser);
        addAndMakeVisible(delay);
        addAndMakeVisible(theVoid);
        addAndMakeVisible(global);
        addAndMakeVisible(pluginTitle);
        addAndMakeVisible(visualiser);

        auto presetNames = processor.presetManager.listPresets();
        int n = 1;
        for (auto presetName: presetNames) {
            presets.addItem(presetName, n++);
        }

        presets.addListener(this);
        addAndMakeVisible(presets);

        presets.setTextWhenNothingSelected("-- presets --");

        bowchurch.setButtonText("bow church");
        vitling.setButtonText("vitling");
        donate.setButtonText("contribute");
        bowchurch.setURL(URL{"https://www.vitling.xyz/ext/crypt/bowchurch"});
        vitling.setURL(URL{"https://www.vitling.xyz/ext/crypt/vitling"});
        donate.setURL(URL{"https://www.vitling.xyz/ext/crypt/donate"});
        
        bowchurch.setFont(fonts.getGothicaBook().withHeight(24.0f), false);
        vitling.setFont(fonts.getGothicaBook().withHeight(24.0f), false);
        donate.setFont(fonts.getGothicaBook().withHeight(24.0f), false);
        bowchurch.setTooltip("");
        vitling.setTooltip("");

        donate.setTooltip("If you find this plugin useful, please contribute a few euros to the development of this and future plugins");

        addAndMakeVisible(bowchurch);
        addAndMakeVisible(vitling);
        addAndMakeVisible(donate);

        save.setButtonText("Save");
        load.setButtonText("Load");
        
        addAndMakeVisible(save);
        addAndMakeVisible(load);

        save.addListener(this);
        load.addListener(this);

        keyboardButton.addListener(this);
        
        addAndMakeVisible(keyboardButton);
        addAndMakeVisible(keyboard);
        keyboard.setVisible(keyboardIsVisible);
        keyboard.setBounds({ 0,625,1000,keyboardHeight });

        versionNumber.setText(String {"v"} + ProjectInfo::versionString, NotificationType::dontSendNotification);
        addAndMakeVisible(versionNumber);

        resized();

    }

    ~CryptAudioProcessorEditor() {
        setLookAndFeel(nullptr);
    }

    void buttonClicked (Button* button) override {
        if (button == &save) {
            openSaveDialog();
        } else if (button == &load) {
            openLoadDialog();
        } else if (button == &keyboardButton) {
            keyboardIsVisible = !keyboardIsVisible;
            if (keyboardIsVisible) {
                setResizeLimits(800,625 + keyboardHeight,1000,625 + keyboardHeight);
            } else {
                setResizeLimits(800,625,1000,625);
            }
            keyboard.setVisible(keyboardIsVisible);
            keyboardButton.setEnabled(keyboardIsVisible);
            resized();
        }
    };

    void openLoadDialog() {
        fileChooser = std::make_unique<FileChooser>("Load preset", File::getSpecialLocation(File::userHomeDirectory), "*.crypt");
        auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this] (const FileChooser& chooser) {
            File file (chooser.getResult());
            if (file.getFileName().isEmpty()) {
                DBG("No file selected");
            } else {
                auto result = XmlDocument::parse(file);
                if (result != nullptr) {
                    if (result->hasTagName(processor.state.state.getType())) {
                        processor.state.replaceState(ValueTree::fromXml(*result));
                        presets.setSelectedId(0, NotificationType::dontSendNotification);
                        presets.setTextWhenNothingSelected(file.getFileName());
                    }
                }
            }
        });
    }

    void openSaveDialog() {
        fileChooser = std::make_unique<FileChooser>("Save preset", File::getSpecialLocation(File::userHomeDirectory), "*.crypt");
        auto flags = FileBrowserComponent::saveMode;
        
        fileChooser->launchAsync(flags, [this] (const FileChooser& chooser) {
            File file (chooser.getResult());
            if (file.getFileName().isEmpty()) {
                DBG("Cancelled save");
            } else {
                DBG("Saving to: " << file.getFullPathName());
                auto currentState = processor.state.copyState();
                std::unique_ptr<XmlElement> xml (currentState.createXml());
                xml->writeTo(file);
                presets.setSelectedId(0, NotificationType::dontSendNotification);
                presets.setTextWhenNothingSelected(file.getFileName());
            }
        });
    }

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override {
        processor.presetManager.applyPreset(comboBoxThatHasChanged->getSelectedId(), processor.state);
    }

    void resized() override {

        auto totalBounds = getLocalBounds();
        auto titleBar = totalBounds.removeFromTop(50);

        if (keyboardIsVisible) {
            auto keyboardBounds = totalBounds.removeFromBottom(keyboardHeight);
            keyboard.setBounds(keyboardBounds);
        }
        
        Rectangle<int> titleText = {titleBar.getCentreX()-102, titleBar.getY(), 200, titleBar.getHeight()};
        pluginTitle.setBounds(titleText);

        Rectangle<int> bcBounds = titleBar.withLeft(titleBar.getWidth() - 250).withWidth(150);
        Rectangle<int> vitBounds = titleBar.withLeft(titleBar.getWidth() - 100);
        Rectangle<int> donateBounds = titleBar.withLeft(titleBar.getWidth() - 350).withWidth(100);
        bowchurch.setBounds(bcBounds);
        vitling.setBounds(vitBounds);
        donate.setBounds(donateBounds);

        presets.setBounds(titleBar.removeFromLeft(200).reduced(10));

        save.setBounds(Rectangle<int>{270,0,70,50}.reduced(10));
        load.setBounds(Rectangle<int>{200,0,70,50}.reduced(10));
        
        
        auto leftBounds = totalBounds.removeFromLeft(400);
        auto rightBounds = totalBounds.removeFromRight(400);

        visualiser.setBounds(totalBounds.withTrimmedTop(-8));

        // I know it seems a bit silly to use StretchableLayoutManagers for each side when they don't actually stretch,
        // but I started with the idea of making it more responsive and only decided to fix it later, and this is working
        // fine still and it lets me switch out orders etc. relatively simply or add responsiveness later
        StretchableLayoutManager leftLayout;
        Component* leftComponents[] = {&osc, &ampEnv, &filter};
        leftLayout.setItemLayout(0, 250,250,250);
        leftLayout.setItemLayout(1, 200,200,200);
        leftLayout.setItemLayout(2, 125,125,125);
        leftLayout.layOutComponents(leftComponents, 3, leftBounds.getX(), leftBounds.getY(), leftBounds.getWidth(), leftBounds.getHeight(), true, true);
        
        auto envBounds = ampEnv.getBounds();
        auto ampEnvBounds = envBounds.removeFromLeft(envBounds.getWidth() / 2);
        ampEnv.setBounds(ampEnvBounds);
        filterEnv.setBounds(envBounds);

        StretchableLayoutManager rightLayout;
        Component* rightComponents[] = {&phaser, &delay, &theVoid, &global};
        rightLayout.setItemLayout(0, 125,125,125);
        rightLayout.setItemLayout(1, 200,200,200);
        rightLayout.setItemLayout(2, 125,125,125);
        rightLayout.setItemLayout(3, 125,125,125);
        rightLayout.layOutComponents(rightComponents, 4, rightBounds.getX(), rightBounds.getY(), rightBounds.getWidth(), rightBounds.getHeight(), true, true);
        global.setBounds(global.getBounds().withTrimmedRight(50));
        keyboardButton.setBounds({global.getRight(), global.getY() + 30, 50, 50});
        versionNumber.setBounds({global.getRight(), global.getY()+90, 50, 20});
    }


    void paint (juce::Graphics& graphics) override {
        auto image = ImageCache::getFromMemory(BinaryData::bg_jpg, BinaryData::bg_jpgSize);
        graphics.drawImageWithin(image, 0, 0, getWidth(), getHeight(), RectanglePlacement::fillDestination);
    }
};