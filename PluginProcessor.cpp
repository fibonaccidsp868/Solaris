#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace solaris;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SolarisProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<P> (juce::ParameterID { "in", 1 },  "Input",  R (-24.f, 24.f, 0.1f), 0.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "out", 1 }, "Output", R (-24.f, 24.f, 0.1f), 0.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "drive", 1 }, "Drive", R (0.f, 100.f, 0.1f), 35.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",    R (0.f, 100.f, 0.1f), 100.f));
    p.push_back (std::make_unique<P> (juce::ParameterID { "clarity", 1 }, "Clarity", R (0.f, 100.f, 0.1f), 35.f));
    p.push_back (std::make_unique<B> (juce::ParameterID { "clarityOn", 1 }, "Clarity On", false));
    p.push_back (std::make_unique<P> (juce::ParameterID { "air", 1 }, "Air", R (0.f, 100.f, 0.1f), 35.f));
    p.push_back (std::make_unique<B> (juce::ParameterID { "airOn", 1 }, "Air On", false));
    p.push_back (std::make_unique<B> (juce::ParameterID { "softclip", 1 }, "Soft Clip", false));
    p.push_back (std::make_unique<B> (juce::ParameterID { "multi", 1 }, "Multiband", false));

    for (int b = 0; b < kNumBands; ++b)
    {
        p.push_back (std::make_unique<P> (juce::ParameterID { "bd" + juce::String (b + 1), 1 },
                                          juce::String (kBandNames[b]) + " Drive", R (0.f, 100.f, 0.1f), 30.f));
        p.push_back (std::make_unique<P> (juce::ParameterID { "bl" + juce::String (b + 1), 1 },
                                          juce::String (kBandNames[b]) + " Level", R (-12.f, 12.f, 0.1f), 0.f));
    }
    return { p.begin(), p.end() };
}

SolarisProcessor::SolarisProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto ptr = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    pIn = ptr ("in"); pOut = ptr ("out"); pDrive = ptr ("drive"); pMix = ptr ("mix");
    pClar = ptr ("clarity"); pClarOn = ptr ("clarityOn"); pAir = ptr ("air"); pAirOn = ptr ("airOn");
    pClip = ptr ("softclip"); pMulti = ptr ("multi");
    for (int b = 0; b < kNumBands; ++b)
    {
        pBandDrive[b] = ptr ("bd" + juce::String (b + 1));
        pBandLevel[b] = ptr ("bl" + juce::String (b + 1));
    }
}

bool SolarisProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo())
           && o == l.getMainInputChannelSet();
}

//==============================================================================
void SolarisProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    sampleRateAtomic.store (fs);
    osFs = fs * 4.0;
    numCh = juce::jmax (1, juce::jmin (2, getTotalNumInputChannels()));

    os = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) samplesPerBlock);
    os->reset();

    const int latency = (int) std::lround (os->getLatencyInSamples());
    setLatencySamples (latency);

    juce::dsp::ProcessSpec spec { fs, (juce::uint32) samplesPerBlock, (juce::uint32) numCh };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuf.setSize (numCh, samplesPerBlock);
    monoTmp.assign ((size_t) samplesPerBlock + 8, 0.0f);

    // crossovers + phase-compensating allpasses (Butterworth Q matches LR4 sum phase)
    const double q = 0.70710678118654752;
    for (int i = 0; i < 4; ++i) { xo[i].set (osFs, kCrossover[i]); xo[i].reset(); }
    ap[0].allPass (osFs, kCrossover[1], q); ap[1].allPass (osFs, kCrossover[2], q); ap[2].allPass (osFs, kCrossover[3], q);
    ap[3].allPass (osFs, kCrossover[2], q); ap[4].allPass (osFs, kCrossover[3], q);
    ap[5].allPass (osFs, kCrossover[3], q);
    for (auto& a : ap) a.reset();

    // Clarity: broad 3.8 kHz presence band.  Air: Maag-style ultra-wide shelves + band-limited exciter
    clarBP.bandPass (osFs, 3800.0, 0.7);
    airHs1.highShelf (osFs, 12000.0, 0.45, 7.0);
    airHs2.highShelf (osFs, juce::jmin (40000.0, osFs * 0.4), 0.5, 5.0);
    airHp.highPass (osFs, 9000.0, 0.7071);
    clarBP.reset(); airHs1.reset(); airHs2.reset(); airHp.reset();

    heatCoef = 1.0 - std::exp (-1.0 / (0.12 * osFs));   // ~120 ms thermal time-constant
    dcR = 1.0 - (6.283185307 * 5.0 / osFs);              // 5 Hz DC blocker
    for (auto& row : shaper) for (auto& s : row) s.reset();
    for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0;

    auto initMul = [&] (Mul& s, float v, double rate) { s.reset (rate, 0.02); s.setCurrentAndTargetValue (v); };
    auto initLin = [&] (Lin& s, float v, double rate) { s.reset (rate, 0.02); s.setCurrentAndTargetValue (v); };
    initMul (smIn, 1.f, osFs); initMul (smOut, 1.f, osFs);
    initLin (smMode, pMulti->load() > 0.5f ? 1.f : 0.f, osFs);
    initLin (smClar, 0.f, osFs); initLin (smAir, 0.f, osFs); initLin (smClip, 0.f, osFs);
    initLin (smMix, 1.f, fs);
    for (auto& s : smDrive) initLin (s, 0.f, osFs);
    for (auto& s : smLevel) initMul (s, 1.f, osFs);
    prevMultiActive = false;
}

//==============================================================================
void SolarisProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (! os || n <= 0 || buffer.getNumChannels() < numCh) return;
    for (int c = numCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (dryBuf.getNumSamples() < n) dryBuf.setSize (numCh, n, false, false, true);

    // latency-matched dry signal
    for (int c = 0; c < numCh; ++c)
    {
        const float* src = buffer.getReadPointer (c);
        float* d = dryBuf.getWritePointer (c);
        for (int i = 0; i < n; ++i) { dryDelay.pushSample (c, src[i]); d[i] = dryDelay.popSample (c); }
    }

    // targets
    smIn.setTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    smOut.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    const bool multi = pMulti->load() > 0.5f;
    smMode.setTargetValue (multi ? 1.f : 0.f);
    const float clarTarget = pClarOn->load() > 0.5f ? pClar->load() * 0.01f : 0.f;
    const float airTarget  = pAirOn->load()  > 0.5f ? pAir->load()  * 0.01f : 0.f;
    const float clipTarget = pClip->load() > 0.5f ? 1.f : 0.f;
    smClar.setTargetValue (clarTarget); smAir.setTargetValue (airTarget); smClip.setTargetValue (clipTarget);
    smMix.setTargetValue (pMix->load() * 0.01f);
    smDrive[0].setTargetValue (pDrive->load() * 0.01f);
    for (int b = 0; b < kNumBands; ++b)
    {
        smDrive[b + 1].setTargetValue (pBandDrive[b]->load() * 0.01f);
        smLevel[b].setTargetValue (juce::Decibels::decibelsToGain (pBandLevel[b]->load()));
    }

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh).getSubBlock (0, (size_t) n);
    auto up = os->processSamplesUp (sub);
    const int osN = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

    constexpr double kDbToNeper = 0.11512925464970229; // ln(10)/20

    for (int i = 0; i < osN; ++i)
    {
        const float inG = smIn.getNextValue(), outG = smOut.getNextValue();
        const float mode = smMode.getNextValue();
        const float cl = smClar.getNextValue(), ai = smAir.getNextValue(), sc = smClip.getNextValue();

        double d[6], g[6], cp[6], k[6];
        for (int s = 0; s < 6; ++s)
        {
            d[s]  = (double) smDrive[s].getNextValue();
            g[s]  = std::exp (d[s] * kMaxDriveDb * kDbToNeper);
            cp[s] = 1.0 / std::sqrt (g[s]);          // partial loudness compensation
            k[s]  = juce::jmin (1.0, d[s] * 20.0);   // exact bypass at zero drive
        }
        float lv[5];
        for (int b = 0; b < 5; ++b) lv[b] = smLevel[b].getNextValue();

        const bool needMulti  = mode > 0.f;
        const bool needSingle = mode < 1.f;
        if (needMulti && ! prevMultiActive)
        {
            for (auto& x : xo) x.reset();
            for (auto& a : ap) a.reset();
        }
        prevMultiActive = needMulti;

        const bool clarActive = cl > 1e-5f || clarTarget > 0.f;
        const bool airActive  = ai > 1e-5f || airTarget > 0.f;

        for (int c = 0; c < numCh; ++c)
        {
            const float x = ch[c][i] * inG;
            float ys = 0.f, ym = 0.f;

            if (needSingle)
            {
                const float s = shaper[0][c].process (x, g[0], d[0], heatCoef) * (float) cp[0];
                ys = x + (float) k[0] * (s - x);
            }

            if (needMulti)
            {
                float b[5], r1, r2, r3, lo;
                xo[0].split (c, x,  lo, r1);
                b[0] = ap[2].process (c, ap[1].process (c, ap[0].process (c, lo)));
                xo[1].split (c, r1, lo, r2);
                b[1] = ap[4].process (c, ap[3].process (c, lo));
                xo[2].split (c, r2, lo, r3);
                b[2] = ap[5].process (c, lo);
                xo[3].split (c, r3, b[3], b[4]);

                for (int s = 0; s < 5; ++s)
                {
                    const float sat = shaper[s + 1][c].process (b[s], g[s + 1], d[s + 1], heatCoef) * (float) cp[s + 1];
                    ym += (b[s] + (float) k[s + 1] * (sat - b[s])) * lv[s];
                }
            }

            float y = ys * (1.f - mode) + ym * mode;

            // DC blocker (removes bias-generated offset)
            const double yd = (double) y - dcX[c] + dcR * dcY[c];
            dcX[c] = (double) y; dcY[c] = yd; y = (float) yd;

            // Clarity: gentle presence lift + odd-harmonic sheen in the 2-7 kHz region
            if (clarActive)
            {
                const float bp = clarBP.process (c, y);
                const float h  = bp - std::tanh (2.5f * bp) / 2.5f;
                y += cl * (0.30f * bp + 1.6f * h);
            }

            // Air: ultra-wide shelves + exciter above 9 kHz
            if (airActive)
            {
                const float a  = airHs2.process (c, airHs1.process (c, y));
                const float e  = airHp.process (c, y);
                const float eh = e - std::tanh (2.0f * e) * 0.5f;
                y += ai * (a - y) + ai * 1.2f * eh;
            }

            y *= outG;
            if (sc > 0.f || clipTarget > 0.f) y += sc * (softClip (y) - y);
            ch[c][i] = y;
        }
    }

    os->processSamplesDown (sub);

    // dry/wet + scope tap
    if ((int) monoTmp.size() < n) monoTmp.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const float m = smMix.getNextValue();
        float mono = 0.f;
        for (int c = 0; c < numCh; ++c)
        {
            float* w = buffer.getWritePointer (c);
            w[i] = dryBuf.getSample (c, i) * (1.f - m) + w[i] * m;
            mono += w[i];
        }
        monoTmp[(size_t) i] = mono / (float) numCh;
    }
    pushScope (monoTmp.data(), n);
}

//==============================================================================
void SolarisProcessor::pushScope (const float* data, int n)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (n, s1, n1, s2, n2);
    if (n1 > 0) std::copy (data, data + n1, fifoData.begin() + s1);
    if (n2 > 0) std::copy (data + n1, data + n1 + n2, fifoData.begin() + s2);
    fifo.finishedWrite (n1 + n2);
}

int SolarisProcessor::pullScope (float* dst, int maxSamples)
{
    const int n = juce::jmin (maxSamples, fifo.getNumReady());
    if (n <= 0) return 0;
    int s1, n1, s2, n2;
    fifo.prepareToRead (n, s1, n1, s2, n2);
    if (n1 > 0) std::copy (fifoData.begin() + s1, fifoData.begin() + s1 + n1, dst);
    if (n2 > 0) std::copy (fifoData.begin() + s2, fifoData.begin() + s2 + n2, dst + n1);
    fifo.finishedRead (n1 + n2);
    return n1 + n2;
}

//==============================================================================
juce::AudioProcessorEditor* SolarisProcessor::createEditor() { return new SolarisEditor (*this); }

void SolarisProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void SolarisProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SolarisProcessor(); }
