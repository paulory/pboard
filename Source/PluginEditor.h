#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
// Colours used across the board
namespace col
{
    const juce::Colour room      { 0xff16171a };
    const juce::Colour board     { 0xff26282c };
    const juce::Colour boardEdge { 0xff0d0e10 };
    const juce::Colour gnaw      { 0xff1b1b1d };
    const juce::Colour swirl     { 0xff8fc3e3 };
    const juce::Colour gold      { 0xffd4b36a };
    const juce::Colour cream     { 0xfff1e6cf };
    const juce::Colour ledRed    { 0xffff3b2f };
    const juce::Colour lcd       { 0xff9fd18b };
}

class BoardLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BoardLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
};

// Rotary knob bound to a parameter. "skin" picks the knob style.
class Knob : public juce::Slider
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& skin);
private:
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// Metal stomp switch bound to a bool parameter.
class Footswitch : public juce::Button
{
public:
    Footswitch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId);
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

// Small toggle (amp power / cab on).
class MiniSwitch : public juce::Button
{
public:
    MiniSwitch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId);
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

// Multi-position selector bound to a choice parameter.
class Selector : public juce::Component
{
public:
    Selector (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray labels,
              juce::Colour text, bool vertical);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    juce::StringArray labels;
    juce::Colour textColour;
    bool vertical;
    int index = 0;
};

// One labelled knob on a panel.
struct LabelledKnob
{
    std::unique_ptr<Knob> knob;
    juce::String label;
};

class Pedal : public juce::Component
{
public:
    Pedal (juce::AudioProcessorValueTreeState& s, juce::String name, juce::String subtitle, juce::Colour body,
           juce::Colour ink, juce::String onParam, juce::StringArray knobIds, juce::StringArray knobLabels,
           juce::String knobSkin);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setSelector (std::unique_ptr<Selector> sel) { selector = std::move (sel); addAndMakeVisible (*selector); }
    juce::Point<float> inputJack() const  { return { getX() + getWidth() - 2.0f, getY() + 120.0f }; }
    juce::Point<float> outputJack() const { return { getX() + 2.0f, getY() + 120.0f }; }
private:
    juce::AudioProcessorValueTreeState& state;
    juce::String name, subtitle, onParamId;
    juce::Colour body, ink;
    std::vector<LabelledKnob> knobs;
    std::unique_ptr<Selector> selector;
    Footswitch footswitch;
};

class AmpHead : public juce::Component
{
public:
    explicit AmpHead (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Point<float> inputJack() const { return { getX() + getWidth() - 40.0f, (float) getBottom() - 4.0f }; }
private:
    juce::AudioProcessorValueTreeState& state;
    std::vector<LabelledKnob> knobs;
    Selector channel;
    MiniSwitch power;
};

class Cabinet : public juce::Component, private juce::Timer
{
public:
    Cabinet (PboardProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    PboardProcessor& proc;
    Knob level;
    MiniSwitch enable;
    juce::TextButton load { "LOAD IR" }, reset { "BUILT-IN" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String irName;
};

class LevelStrip : public juce::Component
{
public:
    LevelStrip (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String title);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setLevel (float linearPeak);
private:
    Knob knob;
    juce::String title;
    float level = 0.0f;
};
} // namespace ui

class PboardEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PboardEditor (PboardProcessor&);
    ~PboardEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PboardProcessor& proc;
    ui::BoardLookAndFeel lnf;

    ui::AmpHead amp;
    ui::Cabinet cab;
    ui::Pedal gnaw, swirl;
    ui::LevelStrip input, output;
    float inLevel = 0.0f, outLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PboardEditor)
};
