#pragma once

#include "Common.h"
#include <vector>

namespace pb
{
// Classic analog bucket-brigade chorus (CE-2 style): a single short delay line
// swept by a rounded-triangle LFO, band-limited before and after the delay like
// a BBD's anti-alias and reconstruction filters, with a touch of saturation,
// mixed with the dry signal.
class BBDChorus
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        size_t n = 1;
        while (n < (size_t) (fs * 0.03) + 8)
            n <<= 1;
        buffer.assign (n, 0.0f);
        mask = n - 1;
        writePos = 0;

        preLP1.setLowpass (fs, 9000.0, 0.54);
        preLP2.setLowpass (fs, 9000.0, 1.31);
        postLP1.setLowpass (fs, 6500.0, 0.54);
        postLP2.setLowpass (fs, 6500.0, 1.31);
        wetHP.setCutoff (60.0, fs);

        rateS.prepare (fs, 50.0);
        depthS.prepare (fs, 50.0);
        mixS.prepare (fs, 30.0);
        rateS.snap(); depthS.snap(); mixS.snap();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        preLP1.reset(); preLP2.reset(); postLP1.reset(); postLP2.reset(); wetHP.reset();
        phase = 0.0;
    }

    // rateHz: LFO speed, depth01: sweep width, mix01: 1 == equal wet/dry (CE-2)
    void setParams (float rateHz, float depth01, float mix01)
    {
        rateS.setTarget (rateHz);
        depthS.setTarget (depth01);
        mixS.setTarget (mix01);
    }

    float process (float x)
    {
        const float rate = rateS.next(), depth = depthS.next(), mix = mixS.next();

        phase += rate / fs;
        if (phase >= 1.0)
            phase -= 1.0;
        const float tri = (float) (2.0 * std::abs (2.0 * phase - 1.0) - 1.0);
        const float lfo = tri * (1.5f - 0.5f * tri * tri);   // rounded triangle

        const float delayMs = 6.0f + lfo * (0.15f + 3.4f * depth);
        const float delaySamples = delayMs * 0.001f * (float) fs;

        const float into = preLP2.process (preLP1.process (x));
        buffer[writePos] = into;

        float wet = readCubic (delaySamples);
        wet = postLP2.process (postLP1.process (wet));
        wet = wetHP.hp (wet);
        wet = std::tanh (wet * 1.1f) / 1.1f;

        writePos = (writePos + 1) & mask;

        return (x + mix * wet) / (1.0f + 0.35f * mix);
    }

private:
    float readCubic (float delaySamples) const
    {
        const float readPos = (float) writePos - delaySamples;
        const float fl = std::floor (readPos);
        const float t = readPos - fl;
        const long i = (long) fl;
        auto at = [this] (long k) { return buffer[(size_t) k & mask]; };
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    double fs = 48000.0, phase = 0.0;
    std::vector<float> buffer;
    size_t mask = 0, writePos = 0;
    Biquad preLP1, preLP2, postLP1, postLP2;
    OnePole wetHP;
    Smoother rateS, depthS, mixS;
};
} // namespace pb
