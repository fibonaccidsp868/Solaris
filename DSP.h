#pragma once
#include <cmath>
#include <algorithm>

namespace solaris
{
constexpr int kNumBands = 5;
// Crossover points: Low | Low-Mid | Mid | High-Mid | High
inline constexpr double kCrossover[4] = { 120.0, 500.0, 2000.0, 6500.0 };
inline constexpr double kEdges[6]     = { 20.0, 120.0, 500.0, 2000.0, 6500.0, 20000.0 };
inline constexpr const char* kBandNames[5] = { "LOW", "LOW MID", "MID", "HIGH MID", "HIGH" };
inline constexpr const char* kBandRanges[5] = { "20-120 Hz", "120-500 Hz", "0.5-2 kHz", "2-6.5 kHz", "6.5-20 kHz" };
constexpr double kMaxDriveDb = 30.0;

//==============================================================================
// Double-precision biquad (RBJ cookbook), transposed direct form II.
// Double precision keeps low-frequency filters (120 Hz at 4x rate) noise-free.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    inline float process (int ch, float xin) noexcept
    {
        const double x = (double) xin;
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    static constexpr double twoPi = 6.283185307179586;

    void lowPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void allPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 - al, -2 * c, 1 + al, 1 + al, -2 * c, 1 - al);
    }
    void bandPass (double fs, double f, double q) // constant 0 dB peak
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void highShelf (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / (2 * q), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq),
             -2 * A * ((A - 1) + (A + 1) * c),
             A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq,
             2 * ((A - 1) - (A + 1) * c),
             (A + 1) - (A - 1) * c - sq);
    }
};

//==============================================================================
// Linkwitz-Riley 4th order crossover: low + high sums to a 2nd-order allpass,
// which is compensated with matching Butterworth allpasses (see processor).
struct LR4
{
    Biquad lp1, lp2, hp1, hp2;
    void set (double fs, double f)
    {
        const double q = 0.70710678118654752;
        lp1.lowPass (fs, f, q);  lp2.lowPass (fs, f, q);
        hp1.highPass (fs, f, q); hp2.highPass (fs, f, q);
    }
    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }
    inline void split (int ch, float x, float& lo, float& hi) noexcept
    {
        lo = lp2.process (ch, lp1.process (ch, x));
        hi = hp2.process (ch, hp1.process (ch, x));
    }
};

//==============================================================================
// "Thermal" saturation core.
//  * Odd-symmetric tanh transfer curve, evaluated with 1st-order antiderivative
//    anti-aliasing (ADAA) on top of 4x oversampling  -> no aliasing "grit".
//  * A slow thermal state (~120 ms) tracks signal energy like a heating junction:
//    it applies a tiny gain sag and a tiny bias shift (adds gentle even harmonics),
//    mimicking how analog stages change character as they warm up.
//  * No noise is generated anywhere; DC from the bias term is removed downstream.
struct Shaper
{
    double uPrev = 0.0, heat = 0.0;

    void reset() { uPrev = 0.0; heat = 0.0; }

    static inline double logcosh (double u) noexcept
    {
        const double a = std::abs (u);
        return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453;
    }

    inline float process (float xin, double g, double driveNorm, double heatCoef) noexcept
    {
        const double u0 = g * (double) xin;
        heat += (u0 * u0 - heat) * heatCoef;
        const double h = std::tanh (0.35 * std::sqrt (heat));  // 0..1 "temperature"
        const double thermal = h * driveNorm;
        const double bias = 0.10 * thermal;
        const double u = u0 * (1.0 - 0.07 * thermal) + bias;

        const double d = u - uPrev;
        double y;
        if (std::abs (d) < 1.0e-6) y = std::tanh (0.5 * (u + uPrev));
        else                       y = (logcosh (u) - logcosh (uPrev)) / d;
        uPrev = u;
        return (float) (y - std::tanh (bias));
    }
};

// Smooth-knee output soft clipper (C1 continuous)
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.80f, ceiling = 0.97f;
    const float a = std::abs (x);
    if (a <= knee) return x;
    const float s = knee + (ceiling - knee) * std::tanh ((a - knee) / (ceiling - knee));
    return x < 0 ? -s : s;
}
} // namespace solaris
