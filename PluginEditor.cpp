#include "PluginEditor.h"
#include <cstring>

using namespace juce;
using namespace solaris;

//==============================================================================
SolarisLnF::SolarisLnF()
{
    setColour (Slider::textBoxTextColourId, ui::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId, ui::dim);
    setColour (TextButton::buttonColourId, ui::panel);
    setColour (TextButton::buttonOnColourId, ui::cyan.withAlpha (0.35f));
    setColour (TextButton::textColourOffId, ui::dim);
    setColour (TextButton::textColourOnId, Colours::white);
}

void SolarisLnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float tw = jmax (3.f, r * 0.085f);
    const bool active = s.isMouseOverOrDragging();

    // tick marks
    g.setColour (Colour (0x26ffffff));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.f;
        const float r0 = r + 1.f, r1 = r - tw * 0.9f;
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.f);
    }
    const float ar = r - tw * 1.8f;

    Path track; track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
    g.setColour (ui::edge);
    g.strokePath (track, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));

    Path val; val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, ang, true);
    // glow
    g.setColour (ui::cyan.withAlpha (active ? 0.28f : 0.16f));
    g.strokePath (val, PathStrokeType (tw * 2.6f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient (ui::cyan, c.x - r, c.y, ui::amber, c.x + r, c.y, false));
    g.strokePath (val, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));

    const float kr = ar - tw * 1.3f;
    // body shadow + brushed-metal body
    g.setColour (Colour (0x66000000));
    g.fillEllipse (Rectangle<float> (kr * 2.1f, kr * 2.1f).withCentre ({ c.x, c.y + kr * 0.08f }));
    g.setGradientFill (ColourGradient (Colour (0xff3a4257), c.x - kr * 0.6f, c.y - kr, Colour (0xff0e1118), c.x + kr * 0.6f, c.y + kr, false));
    g.fillEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c));
    g.setGradientFill (ColourGradient (Colour (0x30ffffff), c.x, c.y - kr, Colour (0x00ffffff), c.x, c.y, false));
    g.fillEllipse (Rectangle<float> (kr * 1.7f, kr * 1.7f).withCentre ({ c.x, c.y - kr * 0.05f }));
    g.setColour (active ? ui::cyan.withAlpha (0.8f) : ui::edge.brighter (0.3f));
    g.drawEllipse (Rectangle<float> (kr * 2, kr * 2).withCentre (c), 1.2f);

    Path p; p.addRoundedRectangle (-1.8f, -kr * 0.9f, 3.6f, kr * 0.36f, 1.6f);
    g.setColour (s.isEnabled() ? Colours::white : ui::dim);
    g.fillPath (p, AffineTransform::rotation (ang).translated (c));
}

void SolarisLnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                                   Slider::SliderStyle, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cx = b.getCentreX();
    const auto col = s.findColour (Slider::thumbColourId);
    g.setColour (ui::edge);
    g.fillRoundedRectangle (Rectangle<float> (5.f, b.getHeight()).withCentre (b.getCentre()), 2.5f);
    g.setGradientFill (ColourGradient (col, cx, sliderPos, col.withAlpha (0.15f), cx, b.getBottom(), false));
    g.fillRoundedRectangle (Rectangle<float> (cx - 2.5f, sliderPos, 5.f, b.getBottom() - sliderPos), 2.5f);
    auto thumb = Rectangle<float> (jmin (34.f, b.getWidth()), 14.f).withCentre ({ cx, sliderPos });
    g.setColour (col.withAlpha (0.25f));
    g.fillRoundedRectangle (thumb.expanded (3.f), 5.f);
    g.setGradientFill (ColourGradient (Colour (0xff3a4258), thumb.getX(), thumb.getY(), Colour (0xff1a1f2b), thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.f);
    g.setColour (col);
    g.drawLine (thumb.getX() + 3, thumb.getCentreY(), thumb.getRight() - 3, thumb.getCentreY(), 1.5f);
}

void SolarisLnF::drawToggleButton (Graphics& g, ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const bool on = b.getToggleState();
    if (on)
    {
        g.setColour (ui::cyan.withAlpha (0.18f));
        g.fillRoundedRectangle (r.expanded (2.f), r.getHeight() * 0.5f);
    }
    g.setColour (on ? ui::cyan.withAlpha (0.30f) : (hl ? ui::edge.brighter (0.2f) : ui::panel));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? ui::cyan : ui::edge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.4f);
    g.setColour (on ? Colours::white : ui::dim);
    g.setFont (jmax (10.f, r.getHeight() * 0.42f));
    g.drawText (b.getButtonText(), r, Justification::centred);
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& a, const String& id, const String& name, const String& suffix, int decimals)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 18);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (12.f)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (a, id, slider);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (18));
    slider.setBounds (b);
}

VFader::VFader (AudioProcessorValueTreeState& a, const String& id, const String& name, const String& suffix,
                int decimals, Colour c)
{
    slider.setSliderStyle (Slider::LinearVertical);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 70, 18);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (decimals);
    slider.setColour (Slider::thumbColourId, c);
    label.setText (name, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (11.f)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (a, id, slider);
}

void VFader::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (16));
    slider.setBounds (b);
}

BandStrip::BandStrip (AudioProcessorValueTreeState& a, int band)
    : index (band),
      drive (a, "bd" + String (band + 1), "DRIVE", "%", 0, ui::bandColours[band]),
      level (a, "bl" + String (band + 1), "LEVEL", " dB", 1, ui::bandColours[band])
{
    addAndMakeVisible (drive);
    addAndMakeVisible (level);
}

void BandStrip::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b, 8.f);
    g.setColour (ui::bandColours[index].withAlpha (0.6f));
    g.fillRoundedRectangle (b.removeFromTop (3.f), 1.5f);
    g.setColour (ui::text);
    g.setFont (13.f);
    g.drawText (kBandNames[index], getLocalBounds().removeFromTop (30).withTrimmedTop (6), Justification::centred);
    g.setColour (ui::dim);
    g.setFont (10.f);
    g.drawText (kBandRanges[index], getLocalBounds().removeFromTop (44).withTrimmedTop (28), Justification::centred);
}

void BandStrip::resized()
{
    auto b = getLocalBounds().reduced (6);
    b.removeFromTop (44);
    drive.setBounds (b.removeFromLeft (b.getWidth() / 2));
    level.setBounds (b);
}

//==============================================================================
SpectrumView::SpectrumView (SolarisProcessor& p)
    : proc (p), fft (fftOrder), window ((size_t) fftSize, dsp::WindowingFunction<float>::hann)
{
    smooth.fill (-120.f);
    frozenSpec.fill (-120.f);
    startTimerHz (30);
}

float SpectrumView::xForFreq (float f) const
{
    return plot.getX() + plot.getWidth() * std::log (jmax (20.f, f) / 20.f) / std::log (1000.f);
}
float SpectrumView::freqForX (float x) const
{
    return 20.f * std::pow (1000.f, (x - plot.getX()) / plot.getWidth());
}
float SpectrumView::yForDb (float db) const
{
    return plot.getY() + plot.getHeight() * (6.f - jlimit (-90.f, 6.f, db)) / 96.f;
}

float SpectrumView::dbAt (const Spec& s, float f0, float f1) const
{
    const float binHz = (float) proc.getSampleRateSafe() / (float) fftSize;
    const float b0 = f0 / binHz, b1 = f1 / binHz;
    int i0 = jlimit (1, numBins - 3, (int) std::floor (b0));
    int i1 = jlimit (i0, numBins - 2, (int) std::ceil (b1));
    if (b1 - b0 < 1.f)
    {
        const float bm = 0.5f * (b0 + b1);
        const int k = jlimit (1, numBins - 3, (int) bm);
        return s[(size_t) k] + (s[(size_t) k + 1] - s[(size_t) k]) * (bm - (float) k);
    }
    float m = -200.f;
    for (int k = i0; k <= i1; ++k) m = jmax (m, s[(size_t) k]);
    return m;
}

std::vector<SpectrumView::Peak> SpectrumView::findPeaks (const Spec& s, float fLo, float fHi, int maxN) const
{
    const float binHz = (float) proc.getSampleRateSafe() / (float) fftSize;
    const int kLo = jmax (2, (int) std::ceil (fLo / binHz));
    const int kHi = jmin (numBins - 3, (int) std::floor (fHi / binHz));
    std::vector<Peak> all;
    for (int k = kLo; k <= kHi; ++k)
    {
        const float a = s[(size_t) k - 1], b = s[(size_t) k], c = s[(size_t) k + 1];
        if (b > a && b >= c && b > -80.f)
        {
            const float den = a - 2.f * b + c;
            const float dl = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
            all.push_back ({ ((float) k + dl) * binHz, b - 0.25f * (a - c) * dl });
        }
    }
    std::sort (all.begin(), all.end(), [] (const Peak& l, const Peak& r) { return l.db > r.db; });
    std::vector<Peak> out;
    for (auto& p : all)
    {
        bool ok = true;
        for (auto& q : out) if (std::abs (std::log2 (p.f / q.f)) < 0.25f) { ok = false; break; }
        if (ok) out.push_back (p);
        if ((int) out.size() >= maxN) break;
    }
    return out;
}

int SpectrumView::rangeForX (float x) const
{
    const float f = freqForX (x);
    for (int i = 0; i < 5; ++i) if (f >= (float) kEdges[i] && f < (float) kEdges[i + 1]) return i;
    return -1;
}

String SpectrumView::fmtFreq (float f)
{
    return f >= 1000.f ? String (f / 1000.f, 2) + " kHz" : String ((int) std::round (f)) + " Hz";
}

void SpectrumView::timerCallback()
{
    float tmp[2048];
    for (;;)
    {
        const int n = proc.pullScope (tmp, 2048);
        if (n <= 0) break;
        std::memmove (win.data(), win.data() + n, sizeof (float) * (size_t) (fftSize - n));
        std::copy (tmp, tmp + n, win.begin() + (fftSize - n));
    }
    if (! frozen)
    {
        std::copy (win.begin(), win.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());
        const float norm = (float) fftSize * 0.25f;
        for (int i = 0; i < numBins; ++i)
        {
            const float db = Decibels::gainToDecibels (fftData[(size_t) i] / norm, -120.f);
            float& s = smooth[(size_t) i];
            s += (db - s) * (db > s ? 0.6f : 0.15f);
        }
    }
    repaint();
}

void SpectrumView::mouseMove (const MouseEvent& e)
{
    hovered = plot.contains (e.position) ? rangeForX (e.position.x) : -1;
    if (hovered >= 0 && ! frozen && ! suppress)
    {
        frozenSpec = smooth;
        frozen = true;
    }
}

void SpectrumView::mouseDown (const MouseEvent&)
{
    frozen = false;
    suppress = true;   // stay live until the pointer leaves and re-enters
}

void SpectrumView::mouseExit (const MouseEvent&)
{
    suppress = false;
    if (! frozen) hovered = -1;
}

void SpectrumView::paint (Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff0e1219), 0.f, 0.f, Colour (0xff080a0f), 0.f, full.getBottom(), false));
    g.fillRoundedRectangle (full, 10.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (full.reduced (0.5f), 10.f, 1.f);

    plot = full.reduced (34.f, 24.f);

    // range shading
    for (int i = 0; i < 5; ++i)
    {
        auto r = Rectangle<float> (xForFreq ((float) kEdges[i]), plot.getY(),
                                   xForFreq ((float) kEdges[i + 1]) - xForFreq ((float) kEdges[i]), plot.getHeight());
        g.setColour (ui::bandColours[i].withAlpha (hovered == i ? 0.16f : 0.045f));
        g.fillRect (r);
    }

    // grid
    g.setFont (10.f);
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xForFreq (f);
        g.setColour (Colour (0x14ffffff));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (ui::dim);
        g.drawText (f >= 1000.f ? String (f / 1000.f, 0) + "k" : String ((int) f),
                    Rectangle<float> (x - 20, plot.getBottom() + 3, 40, 14), Justification::centred);
    }
    for (int db = 0; db >= -84; db -= 12)
    {
        const float y = yForDb ((float) db);
        g.setColour (Colour (0x12ffffff));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (ui::dim);
        g.drawText (String (db), Rectangle<float> (2, y - 7, 30, 14), Justification::right);
    }

    // curve
    const Spec& spec = frozen ? frozenSpec : smooth;
    Path line;
    bool started = false;
    for (float px = 0.f; px <= plot.getWidth(); px += 1.5f)
    {
        const float x = plot.getX() + px;
        const float f0 = freqForX (x), f1 = freqForX (x + 1.5f);
        if (f1 > (float) proc.getSampleRateSafe() * 0.5f) break;
        const float y = yForDb (dbAt (spec, f0, f1));
        if (! started) { line.startNewSubPath (x, y); started = true; }
        else line.lineTo (x, y);
    }
    if (started)
    {
        Path fill (line);
        fill.lineTo (fill.getCurrentPosition().x, plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        const Colour c = frozen ? ui::amber : ui::cyan;
        g.setGradientFill (ColourGradient (c.withAlpha (0.38f), 0.f, plot.getY(), c.withAlpha (0.02f), 0.f, plot.getBottom(), false));
        g.fillPath (fill);
        g.setColour (c.withAlpha (0.18f));
        g.strokePath (line, PathStrokeType (5.f, PathStrokeType::curved));
        if (frozen)
            g.setColour (c);
        else
        {
            ColourGradient cg (ui::bandColours[0], plot.getX(), 0.f, ui::bandColours[4], plot.getRight(), 0.f, false);
            for (int i = 0; i < 5; ++i)
            {
                const float mid = 0.5f * (xForFreq ((float) kEdges[i]) + xForFreq ((float) kEdges[i + 1]));
                cg.addColour (jlimit (0.0, 1.0, (double) ((mid - plot.getX()) / plot.getWidth())), ui::bandColours[i]);
            }
            g.setGradientFill (cg);
        }
        g.strokePath (line, PathStrokeType (1.8f, PathStrokeType::curved));
    }

    // range dividers + headers with live peak
    for (int i = 0; i < 5; ++i)
    {
        const float xa = xForFreq ((float) kEdges[i]), xb = xForFreq ((float) kEdges[i + 1]);
        if (i > 0)
        {
            g.setColour (Colour (0x30ffffff));
            g.drawVerticalLine ((int) xa, plot.getY(), plot.getBottom());
        }
        auto pk = findPeaks (spec, (float) kEdges[i], (float) kEdges[i + 1], 1);
        g.setColour (ui::bandColours[i]);
        g.setFont (11.f);
        g.drawText (kBandNames[i], Rectangle<float> (xa, 4.f, xb - xa, 16.f), Justification::centred);
        if (! pk.empty())
        {
            g.setColour (ui::dim);
            g.setFont (9.5f);
            g.drawText (String (pk[0].db, 1) + " dB", Rectangle<float> (xa, plot.getY() + 2.f, xb - xa, 12.f), Justification::centred);
        }
    }

    // frozen: list the peaks within the hovered range
    if (frozen && hovered >= 0)
    {
        auto peaks = findPeaks (frozenSpec, (float) kEdges[hovered], (float) kEdges[hovered + 1], 4);
        Rectangle<float> lastBox;
        for (auto& p : peaks)
        {
            const float x = xForFreq (p.f), y = yForDb (p.db);
            g.setColour (Colours::white);
            g.fillEllipse (x - 3.5f, y - 3.5f, 7.f, 7.f);
            g.setColour (ui::amber);
            g.drawEllipse (x - 5.f, y - 5.f, 10.f, 10.f, 1.2f);

            auto box = Rectangle<float> (116.f, 18.f).withCentre ({ x, y - 22.f });
            if (box.getX() < plot.getX()) box.setX (plot.getX());
            if (box.getRight() > plot.getRight()) box.setRight (plot.getRight());
            if (box.getY() < plot.getY() + 14.f) box.setY (plot.getY() + 14.f);
            if (! lastBox.isEmpty() && box.intersects (lastBox)) box.setY (lastBox.getBottom() + 2.f);
            lastBox = box;

            g.setColour (Colour (0xe6101520));
            g.fillRoundedRectangle (box, 5.f);
            g.setColour (ui::amber.withAlpha (0.8f));
            g.drawRoundedRectangle (box, 5.f, 1.f);
            g.setColour (Colours::white);
            g.setFont (11.f);
            g.drawText (fmtFreq (p.f) + "   " + String (p.db, 1) + " dB", box, Justification::centred);
        }
        g.setColour (ui::amber);
        g.setFont (10.5f);
        g.drawText ("FROZEN - click to resume live monitoring", Rectangle<float> (plot.getX(), plot.getBottom() - 16.f, plot.getWidth(), 14.f),
                    Justification::centredRight);
    }
}

//==============================================================================
SolarisEditor::SolarisEditor (SolarisProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p),
      inKnob (p.apvts, "in", "INPUT", " dB"),
      outKnob (p.apvts, "out", "OUTPUT", " dB"),
      driveKnob (p.apvts, "drive", "DRIVE", " %", 1),
      mixKnob (p.apvts, "mix", "MIX", " %", 0),
      clarKnob (p.apvts, "clarity", "AMOUNT", " %", 0),
      airKnob (p.apvts, "air", "AMOUNT", " %", 0)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (spectrum);
    for (auto* k : { &inKnob, &outKnob, &driveKnob, &mixKnob, &clarKnob, &airKnob }) addAndMakeVisible (*k);
    for (auto* b : { &clarBtn, &airBtn, &clipBtn }) addAndMakeVisible (*b);
    clarAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "clarityOn", clarBtn);
    airAtt  = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "airOn", airBtn);
    clipAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "softclip", clipBtn);

    for (int i = 0; i < 5; ++i)
    {
        strips[i] = std::make_unique<BandStrip> (p.apvts, i);
        addChildComponent (*strips[i]);
    }

    for (auto* b : { &singleBtn, &multiBtn }) { b->setClickingTogglesState (false); addAndMakeVisible (*b); }
    singleBtn.onClick = [this] { setMulti (false); };
    multiBtn.onClick  = [this] { setMulti (true); };

    multiShown = proc.apvts.getRawParameterValue ("multi")->load() > 0.5f;
    updateMode();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1000.0 / 640.0);
    setResizeLimits (800, 512, 1700, 1088);
    setSize (1000, 640);
    startTimerHz (15);
}

SolarisEditor::~SolarisEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SolarisEditor::setMulti (bool m)
{
    if (auto* prm = proc.apvts.getParameter ("multi"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (m ? 1.f : 0.f);
        prm->endChangeGesture();
    }
    multiShown = m;
    updateMode();
}

void SolarisEditor::timerCallback()
{
    const bool m = proc.apvts.getRawParameterValue ("multi")->load() > 0.5f;
    if (m != multiShown) { multiShown = m; updateMode(); }
}

void SolarisEditor::updateMode()
{
    singleBtn.setToggleState (! multiShown, dontSendNotification);
    multiBtn.setToggleState (multiShown, dontSendNotification);
    driveKnob.setVisible (! multiShown);
    for (auto& s : strips) s->setVisible (multiShown);
    resized();
    repaint();
}

static void drawPanel (Graphics& g, Rectangle<int> r, const String& title, Colour accent)
{
    auto f = r.toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff171b25), 0.f, f.getY(), Colour (0xff0e1118), 0.f, f.getBottom(), false));
    g.fillRoundedRectangle (f, 12.f);
    g.setColour (ui::edge);
    g.drawRoundedRectangle (f.reduced (0.5f), 12.f, 1.f);
    g.setColour (accent.withAlpha (0.8f));
    g.fillRoundedRectangle (Rectangle<float> (f.getX() + 14.f, f.getY() + 9.f, 3.f, 12.f), 1.5f);
    g.setColour (ui::text);
    g.setFont (11.5f);
    g.drawText (title, Rectangle<float> (f.getX() + 24.f, f.getY() + 6.f, f.getWidth() - 30.f, 18.f), Justification::centredLeft);
}

void SolarisEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colour (0xff10141c), 0.f, 0.f, ui::bg, 0.f, (float) getHeight(), false));
    g.fillAll();
    g.setGradientFill (ColourGradient (ui::cyan.withAlpha (0.07f), (float) getWidth() * 0.5f, 0.f, Colours::transparentBlack,
                                       (float) getWidth() * 0.5f, (float) getHeight() * 0.5f, true));
    g.fillAll();

    // header: logo mark + wordmark
    auto top = getLocalBounds().removeFromTop (proportionOfHeight (0.08f)).reduced (18, 6);
    auto logo = top.removeFromLeft (top.getHeight()).toFloat().reduced (3.f);
    g.setColour (ui::amber.withAlpha (0.25f));
    g.fillEllipse (logo.expanded (4.f));
    g.setGradientFill (ColourGradient (ui::amber, logo.getX(), logo.getY(), ui::cyan, logo.getRight(), logo.getBottom(), false));
    g.fillEllipse (logo);
    g.setColour (ui::bg);
    g.fillEllipse (logo.reduced (logo.getWidth() * 0.3f));
    top.removeFromLeft (10);
    g.setColour (Colours::white);
    g.setFont ((float) top.getHeight() * 0.62f);
    g.drawText ("SOLARIS", top.removeFromLeft (proportionOfWidth (0.13f)), Justification::centredLeft);
    g.setColour (ui::cyan);
    g.setFont ((float) top.getHeight() * 0.34f);
    g.drawText ("SATURATOR", top.removeFromLeft (proportionOfWidth (0.14f)), Justification::centredLeft);

    drawPanel (g, leftPanel, "GAIN STAGING", ui::cyan);
    drawPanel (g, centrePanel, multiShown ? "MULTIBAND SATURATION" : "SATURATION", ui::amber);
    drawPanel (g, rightPanel, "ENHANCE / OUTPUT", ui::bandColours[2]);
}

void SolarisEditor::resized()
{
    auto b = getLocalBounds();
    auto top = b.removeFromTop (proportionOfHeight (0.08f)).reduced (18, 10);
    multiBtn.setBounds (top.removeFromRight (proportionOfWidth (0.12f)));
    singleBtn.setBounds (top.removeFromRight (proportionOfWidth (0.12f)));

    spectrum.setBounds (b.removeFromTop (proportionOfHeight (0.40f)).reduced (10, 2));

    auto area = b.reduced (10, 6);
    leftPanel = area.removeFromLeft (proportionOfWidth (0.15f));  area.removeFromLeft (8);
    rightPanel = area.removeFromRight (proportionOfWidth (0.34f)); area.removeFromRight (8);
    centrePanel = area;

    auto left = leftPanel.reduced (8, 0).withTrimmedTop (26);
    inKnob.setBounds (left.removeFromTop (left.getHeight() / 2));
    outKnob.setBounds (left);

    auto right = rightPanel.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6);
    auto row1 = right.removeFromTop (right.getHeight() / 2);
    auto cell = [] (Rectangle<int> r, Knob* k, Component* btn)
    {
        auto btnArea = r.removeFromBottom ((int) (r.getHeight() * 0.24f));
        if (k) k->setBounds (r);
        if (btn) btn->setBounds (btnArea.reduced (btnArea.getWidth() / 6, 2));
    };
    cell (row1.removeFromLeft (row1.getWidth() / 2), &clarKnob, &clarBtn);
    cell (row1, &airKnob, &airBtn);
    mixKnob.setBounds (right.removeFromLeft (right.getWidth() / 2));
    clipBtn.setBounds (right.withSizeKeepingCentre (right.getWidth() * 4 / 5, juce::jmax (28, right.getHeight() / 4)));

    auto c = centrePanel.reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (6);
    driveKnob.setBounds (c.withSizeKeepingCentre (jmin (c.getWidth(), (int) (c.getHeight() * 1.1f)), c.getHeight()));
    const int sw = c.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
        strips[i]->setBounds (c.removeFromLeft (sw));
}
