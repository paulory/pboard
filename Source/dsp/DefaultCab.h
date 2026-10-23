#pragma once

#include "Common.h"
#include <vector>

namespace pb
{
// Synthesises the built-in cabinet impulse response (a closed-back 4x12 style
// voicing) so pboard ships with a usable cab and no third-party IR files:
// low-end resonance, cone break-up peak and notch, steep top-end roll-off and
// a few short early reflections.
inline std::vector<float> makeDefaultCabIR (double fs, int length = 4096)
{
    std::vector<float> ir ((size_t) length, 0.0f);

    Biquad hp1, hp2, res, lowMid, presence, breakup, notch, lp1, lp2, lp3;
    hp1.setHighpass (fs, 70.0, 0.9);
    hp2.setHighpass (fs, 55.0, 0.6);
    res.setPeak (fs, 105.0, 1.6, 4.5);
    lowMid.setPeak (fs, 450.0, 0.9, -3.0);
    presence.setPeak (fs, 1800.0, 1.0, 2.5);
    breakup.setPeak (fs, 3000.0, 2.2, 5.0);
    notch.setPeak (fs, 4400.0, 3.0, -9.0);
    lp1.setLowpass (fs, 5200.0, 0.7);
    lp2.setLowpass (fs, 6000.0, 0.6);
    lp3.setLowpass (fs, 9000.0, 0.5);

    // excitation: direct sound plus a few early reflections
    std::vector<float> excite ((size_t) length, 0.0f);
    excite[0] = 1.0f;
    const double refl[][2] = { { 0.00035, 0.22 }, { 0.00071, -0.12 }, { 0.0013, 0.07 }, { 0.0021, -0.04 } };
    for (auto& r : refl)
    {
        const auto idx = (size_t) (r[0] * fs);
        if (idx < excite.size())
            excite[idx] += (float) r[1];
    }

    for (size_t i = 0; i < ir.size(); ++i)
    {
        float y = excite[i];
        y = hp1.process (y); y = hp2.process (y);
        y = res.process (y); y = lowMid.process (y);
        y = presence.process (y); y = breakup.process (y); y = notch.process (y);
        y = lp1.process (y); y = lp2.process (y); y = lp3.process (y);
        ir[i] = y;
    }

    // fade the tail
    const size_t fadeLen = ir.size() / 4;
    for (size_t i = 0; i < fadeLen; ++i)
        ir[ir.size() - 1 - i] *= (float) i / (float) fadeLen;

    // normalise to unit energy
    double e = 0.0;
    for (auto v : ir) e += (double) v * v;
    const float norm = e > 0.0 ? (float) (1.0 / std::sqrt (e)) : 1.0f;
    for (auto& v : ir) v *= norm;

    return ir;
}
} // namespace pb
