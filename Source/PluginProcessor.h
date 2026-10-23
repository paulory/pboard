#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/AmpSim.h"
#include "dsp/Chorus.h"
#include "dsp/RatPedal.h"

namespace ids
{
    inline constexpr auto ratOn = "rat_on", ratDist = "rat_dist", ratFilter = "rat_filter",
                          ratVol = "rat_vol", ratClip = "rat_clip";
    inline constexpr auto choOn = "cho_on", choRate = "cho_rate", choDepth = "cho_depth", choMix = "cho_mix";
    inline constexpr auto ampOn = "amp_on", ampChannel = "amp_channel", ampGain = "amp_gain",
                          ampBass = "amp_bass", ampMid = "amp_mid", ampTreble = "amp_treble",
                          ampPresence = "amp_presence", ampMaster = "amp_master";
    inline constexpr auto cabOn = "cab_on", cabLevel = "cab_level";
    inline constexpr auto inGain = "in_gain", outLevel = "out_level";
}

class PboardProcessor : public juce::AudioProcessor
{
public:
    PboardProcessor();
    ~PboardProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Cab IR handling (message thread)
    void loadImpulseResponse (const juce::File& file);
    void loadDefaultImpulseResponse();
    juce::String getImpulseResponseName() const;

    juce::AudioProcessorValueTreeState apvts;

    // Input/output meters for the UI (peak, linear)
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateModuleParams();

    static constexpr int kOversamplingOrder = 2;   // 2^2 = 4x

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::Convolution cab;
    juce::AudioBuffer<float> mono, dryCab;

    pb::RatPedal rat;
    pb::BBDChorus chorus;
    pb::AmpSim amp;

    juce::SmoothedValue<float> ratMix, choMix, ampMix, cabMix, inGainS, outGainS, cabLevelS, inputSelect;
    float inEnv[2] { 0.0f, 0.0f };
    int activeInput = 0;

    double currentSampleRate = 48000.0;
    bool firstBlock = true;

    mutable juce::CriticalSection irLock;
    juce::String irName;
    juce::File irFile;

    struct P
    {
        std::atomic<float>* ratOn; std::atomic<float>* ratDist; std::atomic<float>* ratFilter;
        std::atomic<float>* ratVol; std::atomic<float>* ratClip;
        std::atomic<float>* choOn; std::atomic<float>* choRate; std::atomic<float>* choDepth; std::atomic<float>* choMix;
        std::atomic<float>* ampOn; std::atomic<float>* ampChannel; std::atomic<float>* ampGain;
        std::atomic<float>* ampBass; std::atomic<float>* ampMid; std::atomic<float>* ampTreble;
        std::atomic<float>* ampPresence; std::atomic<float>* ampMaster;
        std::atomic<float>* cabOn; std::atomic<float>* cabLevel;
        std::atomic<float>* inGain; std::atomic<float>* outLevel;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PboardProcessor)
};
