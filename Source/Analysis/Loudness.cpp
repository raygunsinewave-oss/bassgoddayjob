#include "Loudness.h"

namespace bgdj
{
void LoudnessMeter::prepare (double sampleRate)
{
    fs = sampleRate;
    {   // K-weighting pre-filter (high shelf), coefficients per BS.1770 for any rate
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        pre.b0 = (Vh + Vb * K / Q + K * K) / a0;
        pre.b1 = 2.0 * (K * K - Vh) / a0;
        pre.b2 = (Vh - Vb * K / Q + K * K) / a0;
        pre.a1 = 2.0 * (K * K - 1.0) / a0;
        pre.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {   // RLB high-pass
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        const double a0 = 1.0 + K / Q + K * K;
        rlb.b0 = 1.0; rlb.b1 = -2.0; rlb.b2 = 1.0;
        rlb.a1 = 2.0 * (K * K - 1.0) / a0;
        rlb.a2 = (1.0 - K / Q + K * K) / a0;
    }
    hopLen = std::max (1, (int) std::lround (fs * 0.1));
    doReset();
}

void LoudnessMeter::doReset() noexcept
{
    hopCount = 0; hopSum = 0.0; hops.fill (0.0); hopIndex = 0; hopsFilled = 0;
    intHist.fill (0); stHist.fill (0);
    rmsAccL = rmsAccR = peakHoldL = peakHoldR = tpMax = 0.0;
    for (auto& c : pre.z1) c = 0; for (auto& c : pre.z2) c = 0;
    for (auto& c : rlb.z1) c = 0; for (auto& c : rlb.z2) c = 0;
    momentary = shortTerm = integrated = -100.0f;
    lra = 0.0f;
    truePeakMaxDb = -100.0f;
}

void LoudnessMeter::process (const float* L, const float* R, int n) noexcept
{
    if (resetRequested.exchange (false))
        doReset();

    const double rmsCoef = std::exp (-1.0 / (fs * 0.3));
    for (int i = 0; i < n; ++i)
    {
        const double l = L[i], r = R[i];
        const double kl = rlb.p (pre.p (l, 0), 0);
        const double kr = rlb.p (pre.p (r, 1), 1);
        hopSum += kl * kl + kr * kr;

        rmsAccL = l * l + rmsCoef * (rmsAccL - l * l);
        rmsAccR = r * r + rmsCoef * (rmsAccR - r * r);
        peakHoldL = std::max (std::abs (l), peakHoldL * 0.99995);
        peakHoldR = std::max (std::abs (r), peakHoldR * 0.99995);

        // true peak estimate via Hermite interpolation at 4x
        const float xs[2] = { (float) l, (float) r };
        for (int c = 0; c < 2; ++c)
        {
            const float xm1 = hist[c][2], x0 = hist[c][1], x1 = hist[c][0], x2 = xs[c];
            for (float t : { 0.25f, 0.5f, 0.75f })
            {
                const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
                tpMax = std::max (tpMax, (double) std::abs (((c3 * t + c2) * t + c1) * t + x0));
            }
            tpMax = std::max (tpMax, (double) std::abs (x2));
            hist[c][2] = hist[c][1]; hist[c][1] = hist[c][0]; hist[c][0] = x2;
        }

        if (++hopCount >= hopLen)
            finishHop();
    }

    auto toDb = [] (double v) { return (float) (20.0 * std::log10 (std::max (v, 1.0e-10))); };
    rmsDbL = toDb (std::sqrt (rmsAccL));
    rmsDbR = toDb (std::sqrt (rmsAccR));
    peakDbL = toDb (peakHoldL);
    peakDbR = toDb (peakHoldR);
    truePeakMaxDb = toDb (tpMax);
    const double rms = std::sqrt (0.5 * (rmsAccL + rmsAccR));
    crestDb = rms > 1.0e-6 ? toDb (std::max (peakHoldL, peakHoldR) / rms) : 0.0f;
}

void LoudnessMeter::finishHop() noexcept
{
    hops[(size_t) hopIndex] = hopSum / hopCount;
    hopIndex = (hopIndex + 1) % (int) hops.size();
    hopsFilled = std::min (hopsFilled + 1, (int) hops.size());
    hopSum = 0.0;
    hopCount = 0;

    auto meanOfLast = [this] (int count)
    {
        double s = 0.0;
        const int c = std::min (count, hopsFilled);
        for (int k = 1; k <= c; ++k)
            s += hops[(size_t) ((hopIndex - k + (int) hops.size()) % (int) hops.size())];
        return c > 0 ? s / c : 0.0;
    };
    auto lufs = [] (double ms) { return ms > 1.0e-12 ? -0.691 + 10.0 * std::log10 (ms) : -100.0; };

    const double m = lufs (meanOfLast (4));
    const double st = lufs (meanOfLast (30));
    momentary = (float) m;
    shortTerm = (float) st;

    if (hopsFilled >= 4 && m > -70.0)
        ++intHist[(size_t) binFor (m)];
    if (hopsFilled >= 30 && st > -70.0)
        ++stHist[(size_t) binFor (st)];

    // integrated: absolute gate -70, relative gate -10 LU
    double eSum = 0.0; uint64_t cnt = 0;
    for (int b = 0; b < kBins; ++b)
        if (intHist[(size_t) b] > 0)
        {
            eSum += intHist[(size_t) b] * std::pow (10.0, (lufsForBin (b) + 0.691) / 10.0);
            cnt += intHist[(size_t) b];
        }
    if (cnt > 0)
    {
        const double rel = lufs (eSum / (double) cnt) - 10.0;
        double e2 = 0.0; uint64_t c2 = 0;
        for (int b = binFor (rel); b < kBins; ++b)
            if (intHist[(size_t) b] > 0)
            {
                e2 += intHist[(size_t) b] * std::pow (10.0, (lufsForBin (b) + 0.691) / 10.0);
                c2 += intHist[(size_t) b];
            }
        integrated = c2 > 0 ? (float) lufs (e2 / (double) c2) : -100.0f;
    }

    // loudness range: short-term values, relative gate -20 LU, P95 - P10
    double se = 0.0; uint64_t sc = 0;
    for (int b = 0; b < kBins; ++b)
        if (stHist[(size_t) b] > 0)
        {
            se += stHist[(size_t) b] * std::pow (10.0, (lufsForBin (b) + 0.691) / 10.0);
            sc += stHist[(size_t) b];
        }
    if (sc > 1)
    {
        const int gate = binFor (lufs (se / (double) sc) - 20.0);
        uint64_t total = 0;
        for (int b = gate; b < kBins; ++b) total += stHist[(size_t) b];
        if (total > 1)
        {
            uint64_t acc = 0; double p10 = 0, p95 = 0; bool got10 = false;
            for (int b = gate; b < kBins; ++b)
            {
                acc += stHist[(size_t) b];
                if (! got10 && acc >= total * 10 / 100) { p10 = lufsForBin (b); got10 = true; }
                if (acc >= total * 95 / 100) { p95 = lufsForBin (b); break; }
            }
            lra = (float) std::max (0.0, p95 - p10);
        }
    }
}
} // namespace bgdj
