#include "Performance/CLPerformanceSummary.h"

double FCLPerformanceSummary::Percentile(const TArray<double>& SortedMs, double Percent)
{
    if (SortedMs.IsEmpty()) return 0;
    const int32 Rank = FMath::CeilToInt(Percent / 100.0 * SortedMs.Num());
    return SortedMs[FMath::Clamp(Rank - 1, 0, SortedMs.Num() - 1)];
}

FCLPerformanceSummary FCLPerformanceSummary::FromFrameMilliseconds(const TArray<double>& FrameMs)
{
    FCLPerformanceSummary Summary;
    Summary.SampleCount = FrameMs.Num();
    if (FrameMs.IsEmpty()) return Summary;

    double Total = 0;
    for (const double Ms : FrameMs)
    {
        Total += Ms;
        if (Ms > Threshold30FPS) ++Summary.FramesOver33;
        if (Ms > Threshold20FPS) ++Summary.FramesOver50;
        if (Ms > ThresholdHitch) ++Summary.FramesOver100;
    }
    Summary.ElapsedSeconds = Total / 1000.0;
    // Frames divided by elapsed time, never the mean of per-frame FPS values.
    Summary.MeanFPS = Summary.ElapsedSeconds > 0 ? FrameMs.Num() / Summary.ElapsedSeconds : 0;

    TArray<double> Sorted = FrameMs;
    Sorted.Sort();
    Summary.MinMs = Sorted[0];
    Summary.MaxMs = Sorted.Last();
    const int32 Count = Sorted.Num();
    Summary.MedianMs = Count % 2 ? Sorted[Count / 2] : (Sorted[Count / 2 - 1] + Sorted[Count / 2]) / 2.0;
    Summary.P95Ms = Percentile(Sorted, 95.0);
    Summary.P99Ms = Percentile(Sorted, 99.0);
    return Summary;
}
