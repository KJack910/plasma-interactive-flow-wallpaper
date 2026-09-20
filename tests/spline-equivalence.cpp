#include "splinetexture.h"

#include <cmath>
#include <cstdio>
#include <chrono>
#include <cstring>
#include <vector>

namespace
{
float hash01Ref(float x)
{
    const float s = std::sin(x) * 43758.5453123f;
    return s - std::floor(s);
}

void generateOriginal(float flow, float *out)
{
    for (int row = 0; row < XmbSpline::height; ++row)
    {
        const float z = (float(row) / float(XmbSpline::height - 1)) * 2.f - 1.f;
        for (int x = 0; x < XmbSpline::width; ++x)
        {
            const float u = float(x) / float(XmbSpline::width - 1);
            const float band = std::sin(flow * 0.25f + z * 1.7f + u * 6.2f) * 0.200f;
            const float secondary = std::cos(z * 7.0f + u * 4.8f + flow * 0.09f) * 0.025f;
            const float travel =
                std::sin((u * 3.14159265358979323846f * 1.3f + z * 0.8f) - flow * 0.25f) * 0.014f * 0.12f +
                std::sin((u * 3.14159265358979323846f * 2.8f - z * 1.2f) + flow * 0.15f) * 0.008f;
            const float perturb = 0.0998587f * 0.07f *
                                  std::sin((u * (4.0f + 0.306001f * 2.0f) + z * 4.0f - flow * 0.6f) * 4.07658f);
            const float descriptor = (hash01Ref(u * 173.0f + z * 47.0f + 13.37f) * 2.f - 1.f) * 0.018f;
            out[size_t(row) * XmbSpline::width + x] =
                0.45f * (band + secondary + descriptor) + 0.55f * (travel + perturb);
        }
    }
}

double bench(void (*fn)(float, float *), const char *label)
{
    static std::vector<float> buffer(XmbSpline::texelCount);
    constexpr int kFrames = 600;
    const auto start = std::chrono::steady_clock::now();
    volatile float benchSeed = 0.0f;
    for (int i = 0; i < kFrames; ++i)
    {
        benchSeed = benchSeed + 0.015625f;
        fn(float(benchSeed), buffer.data());
    }
    const auto end = std::chrono::steady_clock::now();
    const double usPerFrame =
        std::chrono::duration<double, std::micro>(end - start).count() / kFrames;
    std::printf("%-12s %8.1f us/frame  (%d frame)\n", label, usPerFrame, kFrames);
    return usPerFrame;
}
}

int main(int argc, char **argv)
{
    static std::vector<float> reference(XmbSpline::texelCount);
    static std::vector<float> optimized(XmbSpline::texelCount);
    volatile float flowSeed = 0.0f;
    int mismatches = 0;
    for (int f = 0; f <= 64; ++f)
    {
        flowSeed = flowSeed + 0.37f;
        const float flow = float(flowSeed);
        generateOriginal(flow, reference.data());
        XmbSpline::generate(flow, optimized.data());
        for (std::size_t i = 0; i < XmbSpline::texelCount; ++i)
        {
            if (std::memcmp(&reference[i], &optimized[i], sizeof(float)) != 0)
                ++mismatches;
        }
    }
    std::printf("texel confrontati: %zu, diversi: %d\n",
                XmbSpline::texelCount * 65, mismatches);
    if (mismatches != 0)
        return 1;

    if (argc > 1 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        for (int run = 0; run < 3; ++run)
        {
            double oldUs;
            double newUs;
            if (run % 2 == 0)
            {
                oldUs = bench(generateOriginal, "originale");
                newUs = bench(XmbSpline::generate, "ottimizzata");
            }
            else
            {
                newUs = bench(XmbSpline::generate, "ottimizzata");
                oldUs = bench(generateOriginal, "originale");
            }
            std::printf("risparmio: %.1f%%\n", (1.0 - newUs / oldUs) * 100.0);
        }
    }
    return 0;
}
