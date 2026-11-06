#pragma once
#include "PluginProcessor.h"

namespace ui
{
const juce::Colour bg     { 0xff090b10 };
const juce::Colour panel  { 0xff12151d };
const juce::Colour edge   { 0xff252b38 };
const juce::Colour text   { 0xffc9d1e0 };
const juce::Colour dim    { 0xff6c778c };
const juce::Colour cyan   { 0xff4fd8ff };
const juce::Colour amber  { 0xffffb35c };
const juce::Colour bandColours[5] { juce::Colour (0xff4f8dff), juce::Colour (0xff4fd8ff), juce::Colour (0xff5cf2b0),
                                    juce::Colour (0xffffd35c), juce::Colour (0xffff7a5c) };
}

//==============================================================================
class SolarisLnF : public juce::LookAndFeel_V4
{
public:
    SolarisLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
          const juce::String& suffix, int decimals = 1);
    void resized() override;
    juce::Slider slider;
private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class VFader : public juce::Component
{
public:
    VFader (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
            const juce::String& suffix, int decimals, juce::Colour c);
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class BandStrip : public juce::Component
{
public:
    BandStrip (juce::AudioProcessorValueTreeState& apvts, int band);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    int index;
    VFader drive, level;
};

//==============================================================================
// Real-time spectrum with the 5 saturation ranges.  Hover a range -> display freezes
// and the strongest peaks of that range are listed; click to resume live monitoring.
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumView (SolarisProcessor&);
    ~SpectrumView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder, numBins = fftSize / 2;
    using Spec = std::array<float, (size_t) numBins>;

private:
    struct Peak { float f, db; };

    void timerCallback() override;
    float xForFreq (float f) const;
    float freqForX (float x) const;
    float yForDb (float db) const;
    float dbAt (const Spec&, float f0, float f1) const;
    std::vector<Peak> findPeaks (const Spec&, float fLo, float fHi, int maxN) const;
    int rangeForX (float x) const;
    static juce::String fmtFreq (float f);

    SolarisProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, (size_t) fftSize> win {};
    std::array<float, (size_t) fftSize * 2> fftData {};
    Spec smooth {}, frozenSpec {};
    bool frozen = false, suppress = false;
    int hovered = -1;
    juce::Rectangle<float> plot;
};

//==============================================================================
class SolarisEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SolarisEditor (SolarisProcessor&);
    ~SolarisEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setMulti (bool);
    void updateMode();

    SolarisProcessor& proc;
    SolarisLnF lnf;
    SpectrumView spectrum;

    Knob inKnob, outKnob, driveKnob, mixKnob, clarKnob, airKnob;
    juce::ToggleButton clarBtn { "CLARITY" }, airBtn { "AIR" }, clipBtn { "SOFT CLIP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clarAtt, airAtt, clipAtt;
    juce::TextButton singleBtn { "SINGLE" }, multiBtn { "MULTIBAND" };
    std::unique_ptr<BandStrip> strips[5];
    bool multiShown = false;
    juce::Rectangle<int> leftPanel, centrePanel, rightPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolarisEditor)
};
