#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "DSP.h"

class SolarisProcessor : public juce::AudioProcessor
{
public:
    SolarisProcessor();
    ~SolarisProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Solaris Saturator"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lock-free mono tap of the output for the GUI analyser
    int pullScope (float* dst, int maxSamples);
    double getSampleRateSafe() const { return sampleRateAtomic.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void pushScope (const float* data, int n);

    // parameters
    std::atomic<float> *pIn, *pOut, *pDrive, *pMix, *pClar, *pClarOn, *pAir, *pAirOn, *pClip, *pMulti;
    std::atomic<float> *pBandDrive[5], *pBandLevel[5];

    // oversampling / latency
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuf;
    int numCh = 2;
    double fs = 44100.0, osFs = 176400.0, heatCoef = 0.0, dcR = 0.999;

    // smoothers (at oversampled rate unless noted)
    using Lin = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    using Mul = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;
    Mul smIn, smOut;
    Lin smMode, smClar, smAir, smClip, smMix /* base rate */;
    Lin smDrive[6];                 // [0]=single, [1..5]=bands
    Mul smLevel[5];

    // processing state
    solaris::Shaper shaper[6][2];
    solaris::LR4 xo[4];
    solaris::Biquad ap[6];
    solaris::Biquad clarBP, airHs1, airHs2, airHp;
    double dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 };
    bool prevMultiActive = false;

    // scope fifo
    juce::AbstractFifo fifo { 16384 };
    std::vector<float> fifoData = std::vector<float> (16384, 0.0f);
    std::vector<float> monoTmp;
    std::atomic<double> sampleRateAtomic { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolarisProcessor)
};
