#include "Performance/CLPerformanceBenchmark.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogCLPerf, Log, All);

namespace
{
    // Route version 1. Cameras follow the streets the prototype already builds:
    // the jail office and its frontage, Court Street past the drugstore, the
    // courthouse square, then North Lane. Coordinates come from the existing map
    // and the review cameras in the game mode; none of them are new placements.
    const FCLBenchmarkSegment GRoute[] = {
        {TEXT("JailOffice"),     FVector(-120, -310, 155), FVector(-480, 60, 165),    FVector(-20, -110, 115),  FVector(-560, 0, 160),    UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("JailFrontage"),   FVector(-1600, -480, 260), FVector(-1520, 400, 240), FVector(-560, 0, 260),    FVector(-900, 700, 180),  UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("CourtStreet"),    FVector(-1900, 700, 210),  FVector(-1900, 2400, 210),FVector(-950, 900, 200),  FVector(-950, 2600, 220), UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("Drugstore"),      FVector(-1900, 1500, 155), FVector(-1700, 1500, 155),FVector(-1220, 1500, 155),FVector(-1220, 1500, 155),UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("CourthouseSquare"),FVector(-4400, -1400, 450),FVector(-4400, 300, 380),FVector(-3900, 1200, 650),FVector(-3900, 1200, 500),UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("MarketStreet"),   FVector(-5350, -150, 210), FVector(-5350, 2600, 210),FVector(-6550, 1400, 210),FVector(-6550, 3000, 210),UCLPerformanceBenchmark::DefaultSegmentSeconds},
        {TEXT("NorthLane"),      FVector(-5350, 4570, 180), FVector(-2800, 4400, 190),FVector(-5350, 4930, 130),FVector(-2800, 5300, 160),UCLPerformanceBenchmark::DefaultSegmentSeconds}
    };

    // Modes that drive their own camera, fixture state or exit path.
    const TCHAR* GConflictingModes[] = {
        TEXT("CLSmokeTest"), TEXT("CLTestMission"), TEXT("CLTrialSmoke"),
        TEXT("CLTrialReview"), TEXT("CLTownReview"), TEXT("CLStreetReview")
    };

    FString Quoted(const FString& Value)
    {
        FString Escaped = Value;
        Escaped.ReplaceInline(TEXT("\\"), TEXT("/"));
        Escaped.ReplaceInline(TEXT("\""), TEXT("'"));
        return FString::Printf(TEXT("\"%s\""), *Escaped);
    }

    // Values we cannot read stay explicitly unavailable rather than becoming 0 or "".
    FString QuotedOrNull(const FString& Value)
    {
        return Value.TrimStartAndEnd().IsEmpty() ? TEXT("null") : Quoted(Value);
    }

    FString Number(double Value, int32 Digits = 3)
    {
        return FString::Printf(TEXT("%.*f"), Digits, Value);
    }
}

bool UCLPerformanceBenchmark::IsRequested()
{
    return FParse::Param(FCommandLine::Get(), TEXT("CLPerfBenchmark"));
}

bool UCLPerformanceBenchmark::HasConflictingMode(FString* OutMode)
{
    for (const TCHAR* Mode : GConflictingModes)
        if (FParse::Param(FCommandLine::Get(), Mode))
        {
            if (OutMode) *OutMode = Mode;
            return true;
        }
    return false;
}

const FCLBenchmarkSegment* UCLPerformanceBenchmark::Route(int32& OutCount)
{
    OutCount = UE_ARRAY_COUNT(GRoute);
    return GRoute;
}

bool UCLPerformanceBenchmark::ShouldCreateSubsystem(UObject* Outer) const
{
    // Not requested means not built: normal play carries no benchmark object.
    if (!Super::ShouldCreateSubsystem(Outer) || !IsRequested()) return false;
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->WorldType == EWorldType::Game;
}

void UCLPerformanceBenchmark::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    FString Mode;
    if (HasConflictingMode(&Mode))
    {
        Abort(FString::Printf(TEXT("-%s drives its own camera and exit; run the benchmark on its own"), *Mode));
        return;
    }
    if (InWorld.WorldType != EWorldType::Game)
    {
        Abort(TEXT("only a standalone game world is supported"));
        return;
    }
    FParse::Value(FCommandLine::Get(), TEXT("CLPerfWarmUp="), WarmUpSeconds);
    FParse::Value(FCommandLine::Get(), TEXT("CLPerfScale="), SegmentScale);
    if (!(WarmUpSeconds >= 0.0) || !(SegmentScale > 0.0) || WarmUpSeconds > 600.0 || SegmentScale > 20.0)
    {
        Abort(TEXT("warm-up or segment scale outside the supported range"));
        return;
    }
    Start();
}

void UCLPerformanceBenchmark::Start()
{
    UWorld* World = GetWorld();
    APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
    if (!PC)
    {
        Abort(TEXT("no player controller; the benchmark needs a running game world"));
        return;
    }
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Camera = World->SpawnActor<ACameraActor>(GRoute[0].From, (GRoute[0].LookFrom - GRoute[0].From).Rotation(), Params);
    if (!Camera)
    {
        Abort(TEXT("could not spawn the benchmark camera"));
        return;
    }
    Camera->GetCameraComponent()->SetFieldOfView(70.f);

    Controller = PC;
    PreviousViewTarget = PC->GetViewTarget();
    // Input is ignored only for the duration of the run, and restored on any exit.
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->SetViewTarget(Camera);
    if (APawn* Pawn = PC->GetPawn())
    {
        Pawn->SetActorHiddenInGame(true);
        bHidPawn = true;
    }

    State = EState::Running;
    StartTime = FPlatformTime::Seconds();
    LastFrameTime = StartTime;
    RouteSeconds = 0;
    WarmUpElapsed = 0;
    int32 Count = 0;
    Route(Count);
    UE_LOG(LogCLPerf, Display, TEXT("CL_PERF_BEGIN route=%d segments=%d warmup=%.2fs scale=%.2f"), RouteVersion, Count, WarmUpSeconds, SegmentScale);
}

void UCLPerformanceBenchmark::Abort(const FString& Reason)
{
    State = EState::Aborted;
    Restore();
    // Loud, single-line, and non-fatal: the game keeps running normally.
    UE_LOG(LogCLPerf, Error, TEXT("CL_PERF_ABORT %s"), *Reason);
}

void UCLPerformanceBenchmark::Restore()
{
    if (APlayerController* PC = Controller.Get())
    {
        PC->ResetIgnoreMoveInput();
        PC->ResetIgnoreLookInput();
        if (AActor* Previous = PreviousViewTarget.Get()) PC->SetViewTarget(Previous);
        if (bHidPawn)
            if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorHiddenInGame(false);
    }
    bHidPawn = false;
    if (Camera)
    {
        Camera->Destroy();
        Camera = nullptr;
    }
    Controller = nullptr;
    PreviousViewTarget = nullptr;
}

void UCLPerformanceBenchmark::Deinitialize()
{
    // Covers an early quit: input and view are handed back even mid-route.
    if (State == EState::Running) Restore();
    Super::Deinitialize();
}

void UCLPerformanceBenchmark::PositionCamera(double Route) const
{
    if (!Camera) return;
    double Remaining = Route;
    for (const FCLBenchmarkSegment& Segment : GRoute)
    {
        const double Length = Segment.Seconds * SegmentScale;
        if (Remaining > Length && &Segment != &GRoute[UE_ARRAY_COUNT(GRoute) - 1])
        {
            Remaining -= Length;
            continue;
        }
        const double Alpha = Length > 0 ? FMath::Clamp(Remaining / Length, 0.0, 1.0) : 1.0;
        const FVector Position = FMath::Lerp(Segment.From, Segment.To, Alpha);
        const FVector Look = FMath::Lerp(Segment.LookFrom, Segment.LookTo, Alpha);
        Camera->SetActorLocationAndRotation(Position, (Look - Position).Rotation());
        return;
    }
}

void UCLPerformanceBenchmark::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (State != EState::Running) return;

    const double Now = FPlatformTime::Seconds();
    const double FrameSeconds = Now - LastFrameTime;
    LastFrameTime = Now;
    const double SinceStart = Now - StartTime;

    double Total = 0;
    for (const FCLBenchmarkSegment& Segment : GRoute) Total += Segment.Seconds * SegmentScale;

    // Progress accumulates frame durations, clamped per frame, rather than raw
    // wall-clock time. A long stall - a first-run asset load, a shader compile, a
    // breakpoint - would otherwise step the camera past the whole route and end
    // the run with a handful of samples. The stall is still recorded at its true
    // duration; only the camera's progress is clamped.
    const double Step = FMath::Min(FrameSeconds, MaxFrameStepSeconds);
    const bool bWarmUp = WarmUpElapsed < WarmUpSeconds;
    if (bWarmUp) WarmUpElapsed += Step;
    else RouteSeconds += Step;
    PositionCamera(RouteSeconds);

    // The first tick after Start has no previous frame to measure.
    if (FrameSeconds > 0)
    {
        int32 SegmentIndex = 0;
        double Walked = 0;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(GRoute); ++Index)
        {
            Walked += GRoute[Index].Seconds * SegmentScale;
            SegmentIndex = Index;
            if (RouteSeconds <= Walked) break;
        }
        FCLBenchmarkSample Sample;
        Sample.MonotonicSeconds = SinceStart;
        Sample.FrameMs = FrameSeconds * 1000.0;
        Sample.EngineDeltaMs = FApp::GetDeltaTime() * 1000.0;
        Sample.SegmentIndex = bWarmUp ? INDEX_NONE : SegmentIndex;
        Sample.bWarmUp = bWarmUp;
        Recorded.Add(Sample);
    }

    int32 MeasuredCount = 0;
    for (const FCLBenchmarkSample& Sample : Recorded) if (!Sample.bWarmUp) ++MeasuredCount;
    // Percentiles over a handful of frames are meaningless, so hold until the
    // route has produced a usable sample count, with an absolute time limit so a
    // stalled run still ends and reports what it has.
    const bool bRouteDone = RouteSeconds >= Total && MeasuredCount >= MinMeasuredSamples;
    const bool bOutOfTime = SinceStart > (WarmUpSeconds + Total) * 10.0 + 60.0;
    if (bOutOfTime && !bRouteDone)
        UE_LOG(LogCLPerf, Warning, TEXT("CL_PERF_SHORT route ended on the time limit with %d measured samples"), MeasuredCount);
    if (bRouteDone || bOutOfTime) Finish();
}

void UCLPerformanceBenchmark::Finish()
{
    State = EState::Finished;
    TArray<double> Measured;
    for (const FCLBenchmarkSample& Sample : Recorded)
        if (!Sample.bWarmUp) Measured.Add(Sample.FrameMs);
    const FCLPerformanceSummary Summary = FCLPerformanceSummary::FromFrameMilliseconds(Measured);
    const FString Written = WriteResults();
    Restore();

    UE_LOG(LogCLPerf, Display,
        TEXT("CL_PERF_RESULT route=%d samples=%d elapsed=%.3fs mean_fps=%.2f median_ms=%.3f p95_ms=%.3f p99_ms=%.3f over33=%d over50=%d over100=%d"),
        RouteVersion, Summary.SampleCount, Summary.ElapsedSeconds, Summary.MeanFPS,
        Summary.MedianMs, Summary.P95Ms, Summary.P99Ms,
        Summary.FramesOver33, Summary.FramesOver50, Summary.FramesOver100);
    UE_LOG(LogCLPerf, Display, TEXT("CL_PERF_OUTPUT %s"), *Written);
    if (FParse::Param(FCommandLine::Get(), TEXT("CLPerfExit")))
        FPlatformMisc::RequestExitWithStatus(false, 0);
}

FString UCLPerformanceBenchmark::FrameCapText() const
{
    if (!GEngine) return FString();
    if (GEngine->bUseFixedFrameRate) return FString::Printf(TEXT("fixed %.2f"), GEngine->FixedFrameRate);
    const float MaxFPS = GEngine->GetMaxFPS();
    return MaxFPS > 0.f ? FString::Printf(TEXT("%.2f"), MaxFPS) : TEXT("uncapped");
}

FString UCLPerformanceBenchmark::WriteResults() const
{
    const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Performance"), Stamp + TEXT("_") + FPlatformProperties::IniPlatformName());

    // Per-frame rows. gpu_ms is intentionally empty: this build's module
    // dependencies do not expose GPU timing, so it is unavailable, not zero.
    FString Csv = TEXT("frame_index,monotonic_seconds,frame_ms,engine_delta_ms,gpu_ms,phase,segment_index,segment_name\n");
    int32 Index = 0;
    for (const FCLBenchmarkSample& Sample : Recorded)
    {
        const FString SegmentName = Sample.bWarmUp ? TEXT("warmup") : FString(GRoute[FMath::Clamp(Sample.SegmentIndex, 0, static_cast<int32>(UE_ARRAY_COUNT(GRoute)) - 1)].Name);
        Csv += FString::Printf(TEXT("%d,%.6f,%.4f,%.4f,,%s,%d,%s\n"), Index++, Sample.MonotonicSeconds, Sample.FrameMs,
            Sample.EngineDeltaMs, Sample.bWarmUp ? TEXT("warmup") : TEXT("measured"), Sample.SegmentIndex, *SegmentName);
    }

    TArray<double> Measured;
    for (const FCLBenchmarkSample& Sample : Recorded)
        if (!Sample.bWarmUp) Measured.Add(Sample.FrameMs);
    const FCLPerformanceSummary Summary = FCLPerformanceSummary::FromFrameMilliseconds(Measured);

    FIntPoint Resolution(0, 0);
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
        Resolution = GEngine->GameViewport->Viewport->GetSizeXY();

    // Mobile preview is a PC renderer approximation and is labelled as such.
    const bool bMobilePreview = FParse::Param(FCommandLine::Get(), TEXT("CLMobilePreview"));
    const FString PlatformLabel = FString(FPlatformProperties::IniPlatformName()) + (bMobilePreview ? TEXT(" (mobile preview on PC, not a physical Android measurement)") : TEXT(""));

    FString Segments;
    for (int32 I = 0; I < UE_ARRAY_COUNT(GRoute); ++I)
        Segments += FString::Printf(TEXT("%s\n      {\"index\": %d, \"name\": %s, \"seconds\": %s}"),
            I ? TEXT(",") : TEXT(""), I, *Quoted(GRoute[I].Name), *Number(GRoute[I].Seconds * SegmentScale));

    const FString Json = FString::Printf(TEXT(
        "{\n"
        "  \"schema_version\": 1,\n"
        "  \"route_version\": %d,\n"
        "  \"captured_utc\": %s,\n"
        "  \"platform\": %s,\n"
        "  \"mobile_preview\": %s,\n"
        "  \"build_version\": %s,\n"
        "  \"engine_version\": %s,\n"
        "  \"configuration\": %s,\n"
        "  \"resolution\": {\"width\": %d, \"height\": %d},\n"
        "  \"gpu_brand\": %s,\n"
        "  \"device_make_model\": %s,\n"
        "  \"cpu_brand\": %s,\n"
        "  \"frame_cap\": %s,\n"
        "  \"warmup_seconds\": %s,\n"
        "  \"segment_scale\": %s,\n"
        "  \"unavailable\": [\"gpu_ms\", \"render_thread_ms\"],\n"
        "  \"notes\": [\n"
        "    \"Rendered camera route only; no gameplay, AI or UI work is driven.\",\n"
        "    \"GPU and render-thread timings are unavailable in this build and are omitted, not zero.\",\n"
        "    \"Warm-up frames are excluded from every statistic.\"\n"
        "  ],\n"
        "  \"segments\": [%s\n  ],\n"
        "  \"summary\": {\n"
        "    \"sample_count\": %d,\n"
        "    \"elapsed_seconds\": %s,\n"
        "    \"mean_fps\": %s,\n"
        "    \"median_ms\": %s,\n"
        "    \"p95_ms\": %s,\n"
        "    \"p99_ms\": %s,\n"
        "    \"min_ms\": %s,\n"
        "    \"max_ms\": %s,\n"
        "    \"frames_over_33_33ms\": %d,\n"
        "    \"frames_over_50ms\": %d,\n"
        "    \"frames_over_100ms\": %d,\n"
        "    \"warmup_samples\": %d\n"
        "  }\n"
        "}\n"),
        RouteVersion, *Quoted(FDateTime::UtcNow().ToIso8601()), *Quoted(PlatformLabel),
        bMobilePreview ? TEXT("true") : TEXT("false"),
        *QuotedOrNull(FApp::GetBuildVersion()), *QuotedOrNull(FEngineVersion::Current().ToString()),
        *Quoted(LexToString(FApp::GetBuildConfiguration())),
        Resolution.X, Resolution.Y,
        *QuotedOrNull(FPlatformMisc::GetPrimaryGPUBrand()), *QuotedOrNull(FPlatformMisc::GetDeviceMakeAndModel()),
        *QuotedOrNull(FPlatformMisc::GetCPUBrand()), *QuotedOrNull(FrameCapText()),
        *Number(WarmUpSeconds), *Number(SegmentScale), *Segments,
        Summary.SampleCount, *Number(Summary.ElapsedSeconds), *Number(Summary.MeanFPS, 2),
        *Number(Summary.MedianMs), *Number(Summary.P95Ms), *Number(Summary.P99Ms),
        *Number(Summary.MinMs), *Number(Summary.MaxMs),
        Summary.FramesOver33, Summary.FramesOver50, Summary.FramesOver100,
        Recorded.Num() - Summary.SampleCount);

    const FString CsvPath = FPaths::Combine(Folder, TEXT("frames.csv"));
    const FString JsonPath = FPaths::Combine(Folder, TEXT("run.json"));
    if (!FFileHelper::SaveStringToFile(Csv, *CsvPath) || !FFileHelper::SaveStringToFile(Json, *JsonPath))
    {
        UE_LOG(LogCLPerf, Error, TEXT("CL_PERF_ABORT could not write results to %s"), *Folder);
        return TEXT("unwritten");
    }
    return Folder;
}
