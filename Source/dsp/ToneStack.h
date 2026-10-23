#pragma once

#include "Common.h"

namespace pb
{
// Passive treble/mid/bass tone stack (Fender / Marshall topology).
// Transfer function derived by nodal analysis of the circuit:
//   Vin -C1- treble pot (R1, wiper = out) - bass pot (rheostat R2) - mid pot (rheostat R3) - gnd
//   Vin -R4- node N; N -C2- treble-pot bottom; N -C3- mid-pot top
// Discretised with the bilinear transform, evaluated in double precision.
class ToneStack
{
public:
    struct Components { double R1, R2, R3, R4, C1, C2, C3; };

    static constexpr Components marshall { 220e3, 1e6, 22e3, 33e3, 470e-12, 22e-9, 22e-9 };
    static constexpr Components fender   { 250e3, 250e3, 10e3, 100e3, 250e-12, 100e-9, 47e-9 };

    void prepare (double sampleRate) { fs = sampleRate; reset(); }
    void reset() { z1 = z2 = z3 = 0.0; }

    void setComponents (const Components& c) { comp = c; dirty = true; }

    void setControls (float treble01, float mid01, float bass01)
    {
        if (treble01 != t || mid01 != m || bass01 != l)
        {
            t = treble01; m = mid01; l = bass01;
            dirty = true;
        }
    }

    // Call at control rate; recomputes coefficients only when needed.
    void update()
    {
        if (! dirty)
            return;
        dirty = false;

        const auto& c = comp;
        const double R1 = c.R1, R4 = c.R4, C1 = c.C1, C2 = c.C2, C3 = c.C3;
        const double tt = std::clamp ((double) t, 0.0, 1.0);
        const double Rb = std::max ((double) audioTaper (l) * c.R2, 1.0);
        const double Rm = std::max ((double) m * c.R3, 1.0);

        const double b1 = C1*R1*tt + C1*Rb + C1*Rm + C2*Rb + C2*Rm + C3*Rm;
        const double b2 = C1*C2*R1*R4*tt + C1*C2*R1*Rb + C1*C2*R1*Rm + C1*C2*R4*Rb + C1*C2*R4*Rm
                        + C1*C3*R1*R4*tt + C1*C3*R1*Rm + C1*C3*R4*Rb + C1*C3*R4*Rm + C1*C3*Rb*Rm + C2*C3*Rb*Rm;
        const double b3 = C1*C2*C3*Rb*(R1*R4*tt + R1*Rm + R4*Rm);
        const double a1 = C1*R1 + C1*Rb + C1*Rm + C2*R4 + C2*Rb + C2*Rm + C3*R4 + C3*Rm;
        const double a2 = C1*C2*R1*R4 + C1*C2*R1*Rb + C1*C2*R1*Rm + C1*C2*R4*Rb + C1*C2*R4*Rm
                        + C1*C3*R1*R4 + C1*C3*R1*Rm + C1*C3*R4*Rb + C1*C3*R4*Rm + C1*C3*Rb*Rm
                        + C2*C3*R4*Rb + C2*C3*Rb*Rm;
        const double a3 = C1*C2*C3*Rb*(R1*R4 + R1*Rm + R4*Rm);

        const double k = 2.0 * fs, k2 = k * k, k3 = k2 * k;

        const double B0 = b1*k + b2*k2 + b3*k3;
        const double B1 = b1*k - b2*k2 - 3*b3*k3;
        const double B2 = -b1*k - b2*k2 + 3*b3*k3;
        const double B3 = -b1*k + b2*k2 - b3*k3;
        const double A0 = 1 + a1*k + a2*k2 + a3*k3;
        const double A1 = 3 + a1*k - a2*k2 - 3*a3*k3;
        const double A2 = 3 - a1*k - a2*k2 + 3*a3*k3;
        const double A3 = 1 - a1*k + a2*k2 - a3*k3;

        nb0 = B0 / A0; nb1 = B1 / A0; nb2 = B2 / A0; nb3 = B3 / A0;
        na1 = A1 / A0; na2 = A2 / A0; na3 = A3 / A0;
    }

    float process (float xin)
    {
        const double x = xin;
        const double y = nb0 * x + z1;
        z1 = nb1 * x - na1 * y + z2;
        z2 = nb2 * x - na2 * y + z3;
        z3 = nb3 * x - na3 * y;
        return (float) y;
    }

private:
    double fs = 48000.0;
    Components comp = marshall;
    float t = 0.5f, m = 0.5f, l = 0.5f;
    bool dirty = true;
    double nb0 = 0, nb1 = 0, nb2 = 0, nb3 = 0, na1 = 0, na2 = 0, na3 = 0;
    double z1 = 0, z2 = 0, z3 = 0;
};
} // namespace pb
