#pragma once

#include "CoreMinimal.h"

// Summary statistics for a benchmark run. The definitions here are the contract
// the parser in Scripts/Performance reproduces; see Docs/PERFORMANCE_BENCHMARK.md.
//
// - Only measured samples are summarised. Warm-up frames are excluded before any
//   statistic is computed.
// - MeanFPS is total measured frames divided by total measured wall-clock
//   seconds. Instantaneous per-frame FPS values are never averaged.
// - Percentiles use the nearest-rank method on frame durations sorted ascending:
//   index = ceil(P/100 * N) - 1, clamped to [0, N-1]. Median is the middle
//   value for an odd count and the mean of the two middle values for an even one.
// - Thresholds count frames whose duration is strictly greater than the limit.
struct FCLPerformanceSummary
{
    int32 SampleCount = 0;
    double ElapsedSeconds = 0;
    double MeanFPS = 0;
    double MedianMs = 0;
    double P95Ms = 0;
    double P99Ms = 0;
    double MinMs = 0;
    double MaxMs = 0;
    int32 FramesOver33 = 0;   // > 33.33 ms, below 30 FPS
    int32 FramesOver50 = 0;   // > 50 ms, below 20 FPS
    int32 FramesOver100 = 0;  // > 100 ms, a visible hitch

    static constexpr double Threshold30FPS = 33.33;
    static constexpr double Threshold20FPS = 50.0;
    static constexpr double ThresholdHitch = 100.0;

    // FrameMs holds measured frame durations in milliseconds, warm-up already
    // removed. An empty input yields a zeroed summary rather than a divide by zero.
    static FCLPerformanceSummary FromFrameMilliseconds(const TArray<double>& FrameMs);

    // Nearest-rank percentile over an ascending sorted array.
    static double Percentile(const TArray<double>& SortedMs, double Percent);
};
