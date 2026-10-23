#pragma once

#include "Common.h"

namespace pb
{
// Component-level model of the classic RAT-style distortion circuit
// (topology and values as documented in the ElectroSmash circuit analysis).
//
//   input coupling -> non-inverting op-amp gain stage (LM308, with finite
//   gain-bandwidth, slew-rate limit and supply rails) -> coupling cap ->
//   1k series resistor into a pair of antiparallel clipping diodes (solved
//   with Newton-Raphson) -> FILTER RC low-pass -> VOLUME.
//
// Meant to run oversampled (4x in pboard) because of the hard nonlinearities.
class RatPedal
{
public:
    enum class Clip { Silicon = 0, LED = 1, Germanium = 2 };

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        dcIn.setCutoff (7.0, fs);                                     // 22nF into 1M bias
        hpR4.setCutoff (1.0 / (2 * kPi * R4 * C5), fs);               // 47R + 2.2uF  -> ~1.5 kHz
        hpR5.setCutoff (1.0 / (2 * kPi * R5 * C6), fs);               // 560R + 4.7uF -> ~60 Hz
        couple.setCutoff (1.0 / (2 * kPi * 1000.0 * 4.7e-6), fs);     // 4.7uF into 1k + diodes
        slewMax = (float) (kSlewRate / fs);
        volSmooth.prepare (fs, 20.0);
        reset();
        updateCoefficients (true);
    }

    void reset()
    {
        dcIn.reset(); hpR4.reset(); hpR5.reset(); lpRd.reset(); gbw.reset();
        couple.reset(); tone.reset();
        slewState = 0.0f; vDiode = 0.0;
    }

    void setParams (float distortion01, float filter01, float volume01, Clip clip)
    {
        distTarget = distortion01;
        filterTarget = filter01;
        volSmooth.setTarget (audioTaper (volume01) * kOutScale);
        setClip (clip);
    }

    float process (float x)
    {
        if (++counter >= kUpdateEvery)
        {
            counter = 0;
            updateCoefficients (false);
        }

        // --- op-amp gain stage: G(s) = 1 + Zf / Zg ------------------------
        const float v = dcIn.hp (x * kInputVolts);
        const float iGround = hpR4.hp (v) / (float) R4 + hpR5.hp (v) / (float) R5; // current through Zg
        float y = v + rd * lpRd.lp (iGround);                                       // Zf = Rdist || 100pF
        y = gbw.lp (y);                                                             // finite gain-bandwidth

        // supply rails, then slew-rate limiting of the LM308 output
        const float target = rails (y);
        slewState += std::clamp (target - slewState, -slewMax, slewMax);

        // --- clipping diodes behind 1k -----------------------------------
        const float vc = couple.hp (slewState);
        const float clipped = (float) solveDiodes (vc);

        // --- FILTER control: (1k + 1.5k + Rfilter) into 3.3nF ------------
        const float filtered = tone.lp (clipped);

        return filtered * volSmooth.next();
    }

private:
    // Circuit values
    static constexpr double R4 = 47.0, C5 = 2.2e-6;
    static constexpr double R5 = 560.0, C6 = 4.7e-6;
    static constexpr double C4 = 100e-12;
    static constexpr double RdistMax = 100e3;
    static constexpr double RfilterMax = 100e3, Rtone = 1500.0, Rseries = 1000.0, Ctone = 3.3e-9;
    static constexpr double kGBW = 1.0e6;          // LM308 with 30pF compensation
    static constexpr double kSlewRate = 0.3e6;     // 0.3 V/us
    static constexpr float kRail = 4.0f;           // approx. swing around 4.5V bias on 9V
    static constexpr float kInputVolts = 1.0f;     // 1.0 full scale == 1 V
    static constexpr float kOutScale = 5.0f;     // volume at noon ~ unity with the Si diodes
    static constexpr int kUpdateEvery = 16;

    static float rails (float y)
    {
        // smooth saturation into the supply rails
        const float a = std::abs (y) / kRail;
        return y / std::pow (1.0f + a * a * a * a, 0.25f);
    }

    void setClip (Clip c)
    {
        switch (c)
        {
            case Clip::Silicon:   Is = 2.52e-9; nVt = 1.752 * kVt; break;   // 1N914
            case Clip::LED:       Is = 5.0e-17; nVt = 2.0 * kVt;   break;   // red LED (Turbo)
            case Clip::Germanium: Is = 2.0e-7;  nVt = 1.3 * kVt;   break;   // 1N34A
        }
    }

    double solveDiodes (double vin)
    {
        // (vin - V) / R = 2 Is sinh(V / nVt)
        double V = std::clamp (vDiode, std::min (0.0, vin), std::max (0.0, vin));
        for (int it = 0; it < 10; ++it)
        {
            const double e = std::clamp (V / nVt, -80.0, 80.0);
            const double sh = std::sinh (e), ch = std::cosh (e);
            const double f = (vin - V) / Rseries - 2.0 * Is * sh;
            const double df = -1.0 / Rseries - 2.0 * Is * ch / nVt;
            const double step = std::clamp (f / df, -0.25, 0.25);
            V -= step;
            if (std::abs (step) < 1e-7)
                break;
        }
        vDiode = V;
        return V;
    }

    void updateCoefficients (bool snap)
    {
        const float k = snap ? 1.0f : (float) (1.0 - std::exp (-kUpdateEvery / (fs * 0.015)));
        dist += (distTarget - dist) * k;
        filter += (filterTarget - filter) * k;

        rd = (float) (RdistMax * audioTaper (dist) + 50.0);
        lpRd.setCutoff (1.0 / (2 * kPi * rd * C4), fs);

        const double rPar = (R4 * R5) / (R4 + R5);
        const double noiseGain = 1.0 + rd / rPar;
        gbw.setCutoff (kGBW / noiseGain, fs);

        const double rf = RfilterMax * audioTaper (filter);
        tone.setCutoff (1.0 / (2 * kPi * (Rseries + Rtone + rf) * Ctone), fs);

        if (snap)
            volSmooth.snap();
    }

    static constexpr double kVt = 0.02585;

    double fs = 192000.0;
    OnePole dcIn, hpR4, hpR5, lpRd, gbw, couple, tone;
    Smoother volSmooth;
    float distTarget = 0.5f, filterTarget = 0.4f, dist = 0.5f, filter = 0.4f;
    float rd = 10000.0f, slewMax = 1.0f, slewState = 0.0f;
    double Is = 2.52e-9, nVt = 1.752 * kVt, vDiode = 0.0;
    int counter = 0;
};
} // namespace pb
