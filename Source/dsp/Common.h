#pragma once

#include <algorithm>
#include <cmath>

namespace pb
{
constexpr double kPi = 3.14159265358979323846;

inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

// Approximation of an audio (log, "A") taper potentiometer: 10% at half rotation.
inline float audioTaper (float x)
{
    x = std::clamp (x, 0.0f, 1.0f);
    return (std::pow (10.0f, 2.0f * x) - 1.0f) / 99.0f;
}

// First-order topology-preserving (trapezoidal) filter. Matches an analog RC
// network exactly at the cutoff and stays stable when the cutoff is modulated.
struct OnePole
{
    float G = 0.0f, s = 0.0f;

    void setCutoff (double fc, double fs)
    {
        fc = std::clamp (fc, 0.5, 0.45 * fs);
        const double g = std::tan (kPi * fc / fs);
        G = (float) (g / (1.0 + g));
    }

    float lp (float x)
    {
        const float v = (x - s) * G;
        const float y = v + s;
        s = y + v;
        return y;
    }

    float hp (float x) { return x - lp (x); }
    void reset() { s = 0.0f; }
};

// RBJ cookbook biquad, transposed direct form II, double precision state.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    float process (float xin)
    {
        const double x = xin;
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }

    void reset() { z1 = z2 = 0; }

    void setLowpass (double fs, double f, double q)
    {
        const double w = 2 * kPi * std::min (f, 0.45 * fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void setHighpass (double fs, double f, double q)
    {
        const double w = 2 * kPi * std::min (f, 0.45 * fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void setPeak (double fs, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = 2 * kPi * std::min (f, 0.45 * fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }

    void setLowShelf (double fs, double f, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = 2 * kPi * std::min (f, 0.45 * fs) / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sa = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sa), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sa),
             (A + 1) + (A - 1) * c + sa, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sa);
    }

    void setHighShelf (double fs, double f, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = 2 * kPi * std::min (f, 0.45 * fs) / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sa = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sa), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sa),
             (A + 1) - (A - 1) * c + sa, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sa);
    }

private:
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0;
        a1 = na1 / na0; a2 = na2 / na0;
    }
};

// One-pole parameter smoother (exponential glide toward a target).
struct Smoother
{
    float cur = 0.0f, target = 0.0f, coeff = 0.0f;

    void prepare (double fs, double ms) { coeff = (float) std::exp (-1.0 / (fs * ms * 0.001)); }
    void setTarget (float t) { target = t; }
    void snap() { cur = target; }
    float next() { cur = target + (cur - target) * coeff; return cur; }
};
} // namespace pb
