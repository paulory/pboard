// Offline DSP test: renders a synthetic plucked-string signal through every
// module and setting combination at the oversampled rate, and fails on
// NaN/inf, silence, or runaway levels. Prints a level table for calibration.

#include "dsp/AmpSim.h"
#include "dsp/Chorus.h"
#include "dsp/DefaultCab.h"
#include "dsp/RatPedal.h"

#include <cstdio>
#include <random>
#include <vector>

namespace
{
constexpr double kFs = 192000.0;

std::vector<float> pluck (double freq, double seconds, float amp)
{
    const size_t n = (size_t) (kFs * seconds);
    const size_t period = (size_t) (kFs / freq);
    std::vector<float> ks (period);
    std::mt19937 rng (1234);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    for (auto& v : ks) v = d (rng);
    std::vector<float> out (n);
    size_t idx = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const size_t nxt = (idx + 1) % period;
        const float v = ks[idx];
        ks[idx] = 0.996f * 0.5f * (ks[idx] + ks[nxt]);
        idx = nxt;
        out[i] = v * amp;
    }
    return out;
}

struct Stats { double peak = 0, rms = 0; bool finite = true; };

template <typename Fn>
Stats run (const std::vector<float>& in, Fn&& fn)
{
    Stats s;
    double acc = 0;
    for (float x : in)
    {
        const float y = fn (x);
        if (! std::isfinite (y)) s.finite = false;
        s.peak = std::max (s.peak, (double) std::abs (y));
        acc += (double) y * y;
    }
    s.rms = std::sqrt (acc / (double) in.size());
    return s;
}

double db (double v) { return 20.0 * std::log10 (std::max (v, 1e-12)); }

int failures = 0;

void check (const char* name, const Stats& s, double maxPeak = 4.0)
{
    const bool ok = s.finite && s.rms > 1e-5 && s.peak < maxPeak;
    std::printf ("%-44s peak %7.2f dB  rms %7.2f dB  %s\n", name, db (s.peak), db (s.rms), ok ? "ok" : "FAIL");
    if (! ok) ++failures;
}
} // namespace

int main()
{
    const auto guitar = pluck (110.0, 1.5, 0.3f);   // open A, roughly -10 dBFS peaks

    // --- RAT -------------------------------------------------------------
    const char* clipNames[] = { "Si", "LED", "Ge" };
    for (int clip = 0; clip < 3; ++clip)
        for (float dist : { 0.0f, 0.5f, 1.0f })
            for (float filt : { 0.0f, 0.5f, 1.0f })
            {
                pb::RatPedal rat;
                rat.prepare (kFs);
                rat.setParams (dist, filt, 0.5f, (pb::RatPedal::Clip) clip);
                char name[64];
                std::snprintf (name, sizeof name, "RAT %s dist %.1f filter %.1f", clipNames[clip], dist, filt);
                check (name, run (guitar, [&] (float x) { return rat.process (x); }));
            }

    // RAT filter must darken: compare high-frequency energy at filter 0 vs 1
    {
        auto hfEnergy = [&] (float filt)
        {
            pb::RatPedal rat;
            rat.prepare (kFs);
            rat.setParams (0.7f, filt, 0.5f, pb::RatPedal::Clip::Silicon);
            pb::OnePole hp; hp.setCutoff (4000.0, kFs);
            return run (guitar, [&] (float x) { return hp.hp (rat.process (x)); }).rms;
        };
        const double bright = hfEnergy (0.0f), dark = hfEnergy (1.0f);
        std::printf ("RAT filter HF energy: open %.2f dB, closed %.2f dB\n", db (bright), db (dark));
        if (! (dark < bright * 0.3)) { std::printf ("FAIL: filter does not darken\n"); ++failures; }
    }

    // --- Chorus ----------------------------------------------------------
    for (float depth : { 0.0f, 1.0f })
    {
        pb::BBDChorus ch;
        ch.prepare (kFs);
        ch.setParams (8.0f, depth, 1.0f);
        char name[64];
        std::snprintf (name, sizeof name, "Chorus rate 8 depth %.0f mix 1", depth);
        check (name, run (guitar, [&] (float x) { return ch.process (x); }));
    }

    // --- Amp channels ----------------------------------------------------
    for (int channel = 0; channel < 2; ++channel)
        for (float gain : { 0.0f, 0.5f, 1.0f })
            for (float master : { 0.5f, 1.0f })
            {
                pb::AmpSim amp;
                amp.prepare (kFs);
                pb::AmpControls c; c.gain = gain; c.master = master;
                amp.setParams (channel, c);
                amp.snapChannel();
                char name[64];
                std::snprintf (name, sizeof name, "Amp %s gain %.1f master %.1f", channel ? "Crunch" : "Clean", gain, master);
                check (name, run (guitar, [&] (float x) { return amp.process (x); }));
            }

    // --- Full chain: RAT -> chorus -> crunch amp -------------------------
    {
        pb::RatPedal rat; rat.prepare (kFs); rat.setParams (1.0f, 0.0f, 1.0f, pb::RatPedal::Clip::LED);
        pb::BBDChorus ch; ch.prepare (kFs); ch.setParams (8.0f, 1.0f, 1.0f);
        pb::AmpSim amp; amp.prepare (kFs);
        pb::AmpControls c; c.gain = 1.0f; c.master = 1.0f; c.bass = 1.0f; c.treble = 1.0f; c.presence = 1.0f;
        amp.setParams (1, c); amp.snapChannel();
        check ("Full chain, everything maxed", run (guitar, [&] (float x) { return amp.process (ch.process (rat.process (x))); }));
    }

    // --- Default cab -----------------------------------------------------
    {
        const auto ir = pb::makeDefaultCabIR (48000.0);
        bool finite = true;
        for (float v : ir) finite = finite && std::isfinite (v);
        std::printf ("Default cab IR: %zu samples, %s\n", ir.size(), finite ? "ok" : "FAIL");
        if (! finite) ++failures;
    }

    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
