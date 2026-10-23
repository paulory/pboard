#pragma once

#include "Common.h"
#include "ToneStack.h"

namespace pb
{
// Asymmetric triode-like saturation, DC removed afterwards by the stage HPFs.
inline float triode (float x, float bias)
{
    return std::tanh (x + bias) - std::tanh (bias);
}

// Power section: softer, more symmetric (push-pull) compression.
inline float powerAmp (float x)
{
    return x / std::sqrt (1.0f + x * x);
}

struct AmpControls
{
    float gain = 0.5f, bass = 0.5f, mid = 0.5f, treble = 0.5f, presence = 0.5f, master = 0.5f;
};

// American-style clean channel: one gain stage, Fender tone stack, recovery
// stage, lots of headroom.
class CleanChannel
{
public:
    void prepare (double sr)
    {
        fs = sr;
        hpIn.setCutoff (20.0, fs);
        dc1.setCutoff (15.0, fs);
        dc2.setCutoff (15.0, fs);
        miller.setCutoff (14000.0, fs);
        tone.prepare (fs);
        tone.setComponents (ToneStack::fender);
        preS.prepare (fs, 30.0); masterS.prepare (fs, 30.0);
        reset();
    }

    void reset() { hpIn.reset(); dc1.reset(); dc2.reset(); miller.reset(); tone.reset(); presence.reset(); }

    void setControls (const AmpControls& c)
    {
        preS.setTarget (dbToGain (-4.0f + 32.0f * c.gain));
        masterS.setTarget (dbToGain (-24.0f + 36.0f * c.master));
        tone.setControls (c.treble, c.mid, c.bass);
        tone.update();
        if (c.presence != lastPresence)
        {
            lastPresence = c.presence;
            presence.setHighShelf (fs, 3500.0, -4.0 + 12.0 * c.presence);
        }
    }

    float process (float x)
    {
        float y = hpIn.hp (x) * preS.next();
        y = dc1.hp (triode (y * 0.6f, 0.12f)) / 0.6f;
        y = miller.lp (y);
        y = tone.process (y) * kStackMakeup;
        y = dc2.hp (triode (y * 0.35f, 0.08f)) / 0.35f;
        y = presence.process (y * masterS.next() * 0.5f);   // presence acts in the power amp feedback loop
        y = powerAmp (y);
        return y * kOut;
    }

private:
    static constexpr float kStackMakeup = 5.0f;
    static constexpr float kOut = 0.9f;
    double fs = 48000.0;
    OnePole hpIn, dc1, dc2, miller;
    ToneStack tone;
    Biquad presence;
    Smoother preS, masterS;
    float lastPresence = -1.0f;
};

// British-style crunch channel: cascaded gain stages with bright cap, tight
// interstage coupling, cathode follower, Marshall tone stack.
class CrunchChannel
{
public:
    void prepare (double sr)
    {
        fs = sr;
        hpIn.setCutoff (30.0, fs);
        dc1.setCutoff (15.0, fs);
        tight.setCutoff (140.0, fs);
        lp1.setCutoff (10000.0, fs);
        dc2.setCutoff (20.0, fs);
        lp2.setCutoff (7000.0, fs);
        dc3.setCutoff (15.0, fs);
        tone.prepare (fs);
        tone.setComponents (ToneStack::marshall);
        gainS.prepare (fs, 30.0); masterS.prepare (fs, 30.0);
        reset();
    }

    void reset()
    {
        hpIn.reset(); dc1.reset(); tight.reset(); lp1.reset(); dc2.reset(); lp2.reset(); dc3.reset();
        bright.reset(); tone.reset(); presence.reset();
    }

    void setControls (const AmpControls& c)
    {
        gainS.setTarget (dbToGain (-18.0f + 40.0f * c.gain));
        masterS.setTarget (dbToGain (-24.0f + 36.0f * c.master));
        tone.setControls (c.treble, c.mid, c.bass);
        tone.update();
        if (c.gain != lastGain)
        {
            lastGain = c.gain;
            bright.setHighShelf (fs, 2200.0, 7.0 * (1.0f - c.gain));   // bright cap fades as gain goes up
        }
        if (c.presence != lastPresence)
        {
            lastPresence = c.presence;
            presence.setHighShelf (fs, 3500.0, -4.0 + 12.0 * c.presence);
        }
    }

    float process (float x)
    {
        float y = bright.process (hpIn.hp (x));
        y = dc1.hp (triode (y * 2.5f, 0.15f));               // V1a
        y = tight.hp (y) * gainS.next();                     // gain pot between stages
        y = lp1.lp (y);
        y = dc2.hp (triode (y * 1.6f, 0.2f));                // V1b
        y = lp2.lp (y);
        y = dc3.hp (triode (y * 1.4f, 0.25f));               // cathode follower squish
        y = tone.process (y) * kStackMakeup;
        y = presence.process (y * masterS.next() * 0.5f);   // presence acts in the power amp feedback loop
        y = powerAmp (y);
        return y * kOut;
    }

private:
    static constexpr float kStackMakeup = 2.2f;
    static constexpr float kOut = 0.9f;
    double fs = 48000.0;
    OnePole hpIn, dc1, tight, lp1, dc2, lp2, dc3;
    Biquad bright, presence;
    ToneStack tone;
    Smoother gainS, masterS;
    float lastGain = -1.0f, lastPresence = -1.0f;
};

// Two-channel amp with a click-free channel crossfade.
class AmpSim
{
public:
    void prepare (double sr)
    {
        clean.prepare (sr);
        crunch.prepare (sr);
        fadeStep = (float) (1.0 / (sr * 0.03));
    }

    void reset() { clean.reset(); crunch.reset(); }

    void setParams (int channel, const AmpControls& c)
    {
        targetChannel = channel == 0 ? 0.0f : 1.0f;
        clean.setControls (c);
        crunch.setControls (c);
    }

    float process (float x)
    {
        if (fade != targetChannel)
            fade = fade < targetChannel ? std::min (fade + fadeStep, targetChannel)
                                        : std::max (fade - fadeStep, targetChannel);

        if (fade <= 0.0f) return clean.process (x);
        if (fade >= 1.0f) return crunch.process (x);
        return clean.process (x) * (1.0f - fade) + crunch.process (x) * fade;
    }

    void snapChannel() { fade = targetChannel; }

private:
    CleanChannel clean;
    CrunchChannel crunch;
    float fade = 0.0f, targetChannel = 0.0f, fadeStep = 0.001f;
};
} // namespace pb
