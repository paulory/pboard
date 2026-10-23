#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/DefaultCab.h"

using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const char* name, float def)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), def * 10.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 1); }));
}

std::unique_ptr<juce::AudioParameterFloat> decibels (const char* id, const char* name, float lo, float hi, float def)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, 0.1f), def,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 1) + " dB"; }));
}

std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const char* name, bool def)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}
} // namespace

APVTS::ParameterLayout PboardProcessor::createLayout()
{
    APVTS::ParameterLayout layout;

    // Distortion (RAT style). Knobs are 0..10 like the real pedal.
    layout.add (toggle (ids::ratOn, "Gnaw On", true));
    layout.add (knob (ids::ratDist, "Gnaw Distortion", 0.5f));
    layout.add (knob (ids::ratFilter, "Gnaw Filter", 0.35f));
    layout.add (knob (ids::ratVol, "Gnaw Volume", 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::ratClip, 1 }, "Gnaw Diodes", juce::StringArray { "Silicon", "LED", "Germanium" }, 0));

    // Chorus (BBD style)
    layout.add (toggle (ids::choOn, "Swirl On", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::choRate, 1 }, "Swirl Rate", juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.45f), 0.8f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (
            [] (float v, int) { return juce::String (v, 2) + " Hz"; })));
    layout.add (knob (ids::choDepth, "Swirl Depth", 0.6f));
    layout.add (knob (ids::choMix, "Swirl Mix", 1.0f));

    // Amp
    layout.add (toggle (ids::ampOn, "Amp On", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::ampChannel, 1 }, "Amp Channel", juce::StringArray { "Clean", "Crunch" }, 1));
    layout.add (knob (ids::ampGain, "Amp Gain", 0.5f));
    layout.add (knob (ids::ampBass, "Amp Bass", 0.5f));
    layout.add (knob (ids::ampMid, "Amp Middle", 0.5f));
    layout.add (knob (ids::ampTreble, "Amp Treble", 0.6f));
    layout.add (knob (ids::ampPresence, "Amp Presence", 0.5f));
    layout.add (knob (ids::ampMaster, "Amp Master", 0.55f));

    // Cab / IR
    layout.add (toggle (ids::cabOn, "Cab On", true));
    layout.add (decibels (ids::cabLevel, "Cab Level", -24.0f, 12.0f, 0.0f));

    // Global
    layout.add (decibels (ids::inGain, "Input", -24.0f, 24.0f, 0.0f));
    layout.add (decibels (ids::outLevel, "Output", -36.0f, 12.0f, 0.0f));

    return layout;
}

PboardProcessor::PboardProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PBOARD", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p.ratOn = get (ids::ratOn); p.ratDist = get (ids::ratDist); p.ratFilter = get (ids::ratFilter);
    p.ratVol = get (ids::ratVol); p.ratClip = get (ids::ratClip);
    p.choOn = get (ids::choOn); p.choRate = get (ids::choRate); p.choDepth = get (ids::choDepth); p.choMix = get (ids::choMix);
    p.ampOn = get (ids::ampOn); p.ampChannel = get (ids::ampChannel); p.ampGain = get (ids::ampGain);
    p.ampBass = get (ids::ampBass); p.ampMid = get (ids::ampMid); p.ampTreble = get (ids::ampTreble);
    p.ampPresence = get (ids::ampPresence); p.ampMaster = get (ids::ampMaster);
    p.cabOn = get (ids::cabOn); p.cabLevel = get (ids::cabLevel);
    p.inGain = get (ids::inGain); p.outLevel = get (ids::outLevel);

    loadDefaultImpulseResponse();
}

bool PboardProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk;
}

void PboardProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const auto maxBlock = (juce::uint32) juce::jmax (1, samplesPerBlock);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        1, kOversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing (maxBlock);
    const double osRate = sampleRate * oversampling->getOversamplingFactor();

    rat.prepare (osRate);
    chorus.prepare (osRate);
    amp.prepare (osRate);

    cab.prepare ({ sampleRate, maxBlock, 1 });

    mono.setSize (1, (int) maxBlock);
    dryCab.setSize (1, (int) maxBlock);

    for (auto* s : { &ratMix, &choMix, &ampMix, &cabMix })
        s->reset (sampleRate, 0.02);
    for (auto* s : { &inGainS, &outGainS, &cabLevelS })
        s->reset (sampleRate, 0.05);
    inputSelect.reset (sampleRate, 0.03);
    inputSelect.setCurrentAndTargetValue ((float) activeInput);

    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));
    firstBlock = true;
}

void PboardProcessor::updateModuleParams()
{
    const auto tenth = [] (std::atomic<float>* v) { return v->load() * 0.1f; };

    rat.setParams (tenth (p.ratDist), tenth (p.ratFilter), tenth (p.ratVol),
                   (pb::RatPedal::Clip) juce::jlimit (0, 2, (int) p.ratClip->load()));
    chorus.setParams (p.choRate->load(), tenth (p.choDepth), tenth (p.choMix));

    pb::AmpControls c;
    c.gain = tenth (p.ampGain); c.bass = tenth (p.ampBass); c.mid = tenth (p.ampMid);
    c.treble = tenth (p.ampTreble); c.presence = tenth (p.ampPresence); c.master = tenth (p.ampMaster);
    amp.setParams ((int) p.ampChannel->load(), c);

    ratMix.setTargetValue (p.ratOn->load() > 0.5f ? 1.0f : 0.0f);
    choMix.setTargetValue (p.choOn->load() > 0.5f ? 1.0f : 0.0f);
    ampMix.setTargetValue (p.ampOn->load() > 0.5f ? 1.0f : 0.0f);
    cabMix.setTargetValue (p.cabOn->load() > 0.5f ? 1.0f : 0.0f);
    inGainS.setTargetValue (juce::Decibels::decibelsToGain (p.inGain->load()));
    outGainS.setTargetValue (juce::Decibels::decibelsToGain (p.outLevel->load()));
    cabLevelS.setTargetValue (juce::Decibels::decibelsToGain (p.cabLevel->load()));

    if (firstBlock)
    {
        for (auto* s : { &ratMix, &choMix, &ampMix, &cabMix, &inGainS, &outGainS, &cabLevelS })
            s->setCurrentAndTargetValue (s->getTargetValue());
        amp.snapChannel();
        firstBlock = false;
    }
}

void PboardProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numOut = getTotalNumOutputChannels();
    if (numSamples == 0 || oversampling == nullptr)
        return;

    updateModuleParams();

    const int maxBlock = mono.getNumSamples();
    float inPk = 0.0f, outPk = 0.0f;

    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, numSamples - start);

        // Guitar is mono: follow whichever input channel carries the signal
        // (guitar in input 1 or 2 both work), crossfading on a change.
        auto* m = mono.getWritePointer (0);
        const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
        if (numIn >= 2)
        {
            const float pk0 = buffer.getMagnitude (0, start, n), pk1 = buffer.getMagnitude (1, start, n);
            inEnv[0] = juce::jmax (pk0, inEnv[0] * 0.97f);
            inEnv[1] = juce::jmax (pk1, inEnv[1] * 0.97f);
            if (inEnv[1] > inEnv[0] * 2.0f)       activeInput = 1;   // hysteresis: switch only on a clear winner
            else if (inEnv[0] > inEnv[1] * 2.0f)  activeInput = 0;
            inputSelect.setTargetValue ((float) activeInput);

            const auto* c0 = buffer.getReadPointer (0, start);
            const auto* c1 = buffer.getReadPointer (1, start);
            for (int i = 0; i < n; ++i)
            {
                const float w = inputSelect.getNextValue();
                m[i] = (c0[i] + w * (c1[i] - c0[i])) * inGainS.getNextValue();
            }
        }
        else
        {
            juce::FloatVectorOperations::copy (m, buffer.getReadPointer (0, start), n);
            for (int i = 0; i < n; ++i)
                m[i] *= inGainS.getNextValue();
        }
        inPk = juce::jmax (inPk, juce::FloatVectorOperations::findMaximum (m, n),
                           -juce::FloatVectorOperations::findMinimum (m, n));

        // Pedals and amp run 4x oversampled.
        juce::dsp::AudioBlock<float> block (mono.getArrayOfWritePointers(), 1, (size_t) n);
        auto up = oversampling->processSamplesUp (block);
        auto* u = up.getChannelPointer (0);
        const auto upN = (int) up.getNumSamples();
        const int factor = (int) oversampling->getOversamplingFactor();

        for (int i = 0; i < upN; ++i)
        {
            if (i % factor == 0)
            {
                ratMix.getNextValue(); choMix.getNextValue(); ampMix.getNextValue();
            }
            const float rm = ratMix.getCurrentValue(), cm = choMix.getCurrentValue(), am = ampMix.getCurrentValue();

            float x = u[i];
            x = rm >= 1.0f ? rat.process (x) : (rm <= 0.0f ? x : x + rm * (rat.process (x) - x));
            x = cm >= 1.0f ? chorus.process (x) : (cm <= 0.0f ? x : x + cm * (chorus.process (x) - x));
            const float a = amp.process (x);          // keep amp state warm for click-free bypass
            u[i] = x + am * (a - x);
        }

        oversampling->processSamplesDown (block);

        // Cabinet IR at the base rate.
        juce::FloatVectorOperations::copy (dryCab.getWritePointer (0), m, n);
        {
            juce::dsp::AudioBlock<float> cabBlock (mono.getArrayOfWritePointers(), 1, (size_t) n);
            cab.process (juce::dsp::ProcessContextReplacing<float> (cabBlock));
        }
        const auto* dry = dryCab.getReadPointer (0);
        for (int i = 0; i < n; ++i)
        {
            const float c = cabMix.getNextValue();
            const float wet = m[i] * cabLevelS.getNextValue();
            float y = (dry[i] + c * (wet - dry[i])) * outGainS.getNextValue();
            // gentle safety limiter above about -1 dBFS
            if (std::abs (y) > 0.89f)
                y = std::copysign (0.89f + 0.11f * std::tanh ((std::abs (y) - 0.89f) / 0.11f), y);
            m[i] = y;
        }
        outPk = juce::jmax (outPk, juce::FloatVectorOperations::findMaximum (m, n),
                            -juce::FloatVectorOperations::findMinimum (m, n));

        for (int ch = 0; ch < numOut; ++ch)
            buffer.copyFrom (ch, start, mono, 0, 0, n);
    }

    inputPeak.store (juce::jmax (inPk, inputPeak.load()));
    outputPeak.store (juce::jmax (outPk, outputPeak.load()));
}

// ---------------------------------------------------------------------------

void PboardProcessor::loadImpulseResponse (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    cab.loadImpulseResponse (file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0,
                             juce::dsp::Convolution::Normalise::yes);
    const juce::ScopedLock sl (irLock);
    irFile = file;
    irName = file.getFileNameWithoutExtension();
}

void PboardProcessor::loadDefaultImpulseResponse()
{
    constexpr double irRate = 48000.0;
    const auto ir = pb::makeDefaultCabIR (irRate);
    juce::AudioBuffer<float> buf (1, (int) ir.size());
    buf.copyFrom (0, 0, ir.data(), (int) ir.size());
    cab.loadImpulseResponse (std::move (buf), irRate, juce::dsp::Convolution::Stereo::no,
                             juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
    const juce::ScopedLock sl (irLock);
    irFile = juce::File();
    irName = "Built-in 4x12";
}

juce::String PboardProcessor::getImpulseResponseName() const
{
    const juce::ScopedLock sl (irLock);
    return irName;
}

void PboardProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::ScopedLock sl (irLock);
        state.setProperty ("irPath", irFile.getFullPathName(), nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PboardProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    const juce::String path = state.getProperty ("irPath", "").toString();
    apvts.replaceState (state);

    if (juce::File::isAbsolutePath (path) && juce::File (path).existsAsFile())
        loadImpulseResponse (juce::File (path));
    else
        loadDefaultImpulseResponse();
}

juce::AudioProcessorEditor* PboardProcessor::createEditor()
{
    return new PboardEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PboardProcessor();
}
