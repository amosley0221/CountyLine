#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Stats/Stats.h"
#include "Tickable.h"
#include "Performance/CLPerformanceSummary.h"
#include "CLPerformanceBenchmark.generated.h"

// One leg of the camera route. The camera moves from From to To while looking
// from LookFrom to LookTo, at constant speed over Seconds. Coordinates are the
// existing map's; nothing here moves geometry or the player's saved location.
struct FCLBenchmarkSegment
{
    const TCHAR* Name;
    FVector From;
    FVector To;
    FVector LookFrom;
    FVector LookTo;
    double Seconds;
};

// One recorded frame. Durations are wall-clock milliseconds from a monotonic
// clock; EngineDeltaMs is what the engine reported for the same frame.
struct FCLBenchmarkSample
{
    double MonotonicSeconds = 0;
    double FrameMs = 0;
    double EngineDeltaMs = 0;
    int32 SegmentIndex = 0;
    bool bWarmUp = false;
};

// Opt-in rendered-scene benchmark. It exists only when -CLPerfBenchmark is on the
// command line, so normal play never constructs it, and it refuses to run in any
// world or mode it does not support.
//
// This measures a fixed camera route through already-built geometry. It is not a
// substitute for gameplay profiling: no AI, input, combat or paper UI is driven.
UCLASS()
class COUNTYLINE_API UCLPerformanceBenchmark : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    // Bump when the route, its timings or the recorded columns change, so old
    // captures are never compared against a different path.
    static constexpr int32 RouteVersion = 1;
    static constexpr double DefaultWarmUpSeconds = 5.0;
    static constexpr double DefaultSegmentSeconds = 6.0;
    // A single frame never advances the route by more than this, so one stall
    // cannot skip the camera through the town.
    static constexpr double MaxFrameStepSeconds = 0.25;
    // Below this many measured frames, percentiles are not worth reporting.
    static constexpr int32 MinMeasuredSamples = 60;

    // True when the operator asked for a benchmark on the command line.
    static bool IsRequested();
    // Modes whose own cameras, fixtures or exit behaviour would collide.
    static bool HasConflictingMode(FString* OutMode = nullptr);

    static const FCLBenchmarkSegment* Route(int32& OutCount);

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCLPerformanceBenchmark, STATGROUP_Tickables); }

    bool IsRunning() const { return State == EState::Running; }
    const TArray<FCLBenchmarkSample>& Samples() const { return Recorded; }

private:
    enum class EState : uint8 { Idle, Running, Finished, Aborted };

    void Start();
    void Abort(const FString& Reason);
    void Finish();
    void Restore();
    void PositionCamera(double RouteSeconds) const;
    FString WriteResults() const;
    FString FrameCapText() const;

    EState State = EState::Idle;
    double WarmUpSeconds = DefaultWarmUpSeconds;
    double SegmentScale = 1.0;
    double StartTime = 0;
    double LastFrameTime = 0;
    double RouteSeconds = 0;
    double WarmUpElapsed = 0;
    TArray<FCLBenchmarkSample> Recorded;
    UPROPERTY() TObjectPtr<class ACameraActor> Camera;
    TWeakObjectPtr<class APlayerController> Controller;
    TWeakObjectPtr<class AActor> PreviousViewTarget;
    bool bHidPawn = false;
};
