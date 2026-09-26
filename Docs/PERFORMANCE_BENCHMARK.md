# Performance benchmark harness

Opt-in, repeatable frame-time capture for a fixed camera route through Pecos Bend, plus a parser that turns a capture into a summary. Measurement only: it adds no art, no gameplay and no renderer settings.

Branch `claude/performance-benchmark`, based on `feature/street-supporting-cast` at `a3771c6`.

## What it is and is not

- It measures **a rendered camera route over existing geometry**. No input, AI, dialogue, paper UI or save traffic is driven, so it is **not** a substitute for gameplay profiling or a frame budget for real play.
- It is **opt-in**. Without `-CLPerfBenchmark` the subsystem is never created, so normal play is untouched.
- It **never reads or writes the player's save**. `-CLPerfBenchmark` joins `-CLSmokeTest` and `-CLTestMission` in the save-isolation guard in `CLCaseState.cpp`: the slot is not loaded at startup and `WriteDate` refuses to write.
- A PC run with `-CLMobilePreview` is a **mobile preview on PC**, never an Android result. The capture metadata and the parser's report both say so.

## Route version 1

Seven legs, each 6 seconds by default, through geometry the prototype already builds. Coordinates come from the existing map and the review cameras in `CLPrototypeGameMode.cpp`; no placement was added or moved.

| # | Segment | Covers |
| --- | --- | --- |
| 0 | `JailOffice` | Office interior, desk and report |
| 1 | `JailFrontage` | Jail exterior and the street approach |
| 2 | `CourtStreet` | Court Street spine past the eastern shopfronts |
| 3 | `Drugstore` | Slow push onto the drugstore frontage |
| 4 | `CourthouseSquare` | Courthouse mass, cupola and forecourt |
| 5 | `MarketStreet` | Western commercial block |
| 6 | `NorthLane` | Residential lane and homes |

`RouteVersion` is recorded in every capture. **Bump it whenever the path, its timings or the recorded columns change**, so captures from different routes are never compared.

Warm-up is separate: for the first `-CLPerfWarmUp=` seconds (default 5) the camera holds the route start and frames are recorded with `phase=warmup`. Warm-up frames are excluded from every statistic. They are kept in the CSV so shader-compile and streaming spikes stay visible.

The camera is parameterised by elapsed time, not by frame index, so the path through space is identical at any frame rate. Which points along it get sampled does depend on frame timing.

Route progress accumulates frame durations **clamped to 0.25 s per frame**, rather than raw wall-clock time. Without that clamp a single long stall — a first-run asset load, a shader compile, a breakpoint — steps the camera past the entire route and ends the run with a couple of samples. The stall itself is still recorded at its true duration; only the camera's progress is clamped. A run also refuses to finish before **60 measured frames**, since percentiles over fewer are not worth reporting, and it gives up on an absolute time limit, logging `CL_PERF_SHORT` with the count it managed.

## Metrics and their definitions

Computed on measured frames only, by `Scripts/Performance/summarize_benchmark.py` (authoritative) and mirrored in `FCLPerformanceSummary`:

- `sample_count` — measured frames.
- `elapsed_seconds` — sum of measured frame durations.
- `mean_fps` — `sample_count / elapsed_seconds`. **Instantaneous per-frame FPS is never averaged**; one 500 ms stall pulls this number down as it should.
- `median_ms` — middle value for an odd count, mean of the two middle values for an even one.
- `p95_ms`, `p99_ms` — **nearest-rank** on durations sorted ascending: `index = ceil(P/100 * N) - 1`, clamped to `[0, N-1]`. With 100 samples p95 is the 95th slowest frame. No interpolation.
- `frames_over_33_33ms`, `frames_over_50ms`, `frames_over_100ms` — counts **strictly greater** than 33.33 / 50 / 100 ms, i.e. below 30 / 20 / 10 FPS. A frame exactly on a limit is not counted.
- `min_ms`, `max_ms`, and the same figures per segment.
- `engine_delta_ms_mean` — the engine's own reported delta, for comparison with the wall clock.

Frame durations come from `FPlatformTime::Seconds()`, a monotonic clock, measured tick to tick on the game thread.

### Unavailable values stay unavailable

`gpu_ms` and render-thread time are **not** captured. Those globals live in `RenderCore`, which this module does not depend on, and `CountyLine.Build.cs` is outside this task's scope. The CSV column is written empty, `run.json` lists them under `"unavailable"`, and the parser reports `unavailable` — never `0`. **For Codex:** adding `"RenderCore"` to `CountyLine.Build.cs` would let `GGPUFrameTime` / `GRenderThreadTime` be recorded; that is a one-line module change I did not make.

Metadata that cannot be read (build version on a local build, device make and model on desktop) is written as JSON `null` and printed as `unavailable`. Captures carry no account name, serial number, credential or unrelated path.

## Running on PC

Build the editor target first, then:

```bash
pwsh -File "Scripts/Performance/Run-Benchmark.ps1" -EngineRoot "F:/Vacancy/Unreal/UE_5.8"
```

Useful switches: `-ResX/-ResY`, `-WarmUpSeconds`, `-SegmentScale` (stretches every leg), `-FrameCap` (0 = uncapped, the default and usually what you want), `-MobilePreview`, `-KeepOpen` (skip the automatic exit), `-TimeoutSeconds`.

The script launches the game, waits for the route, finds the new capture and summarizes it. Run it on an idle machine: do not build, cook or package at the same time, and leave the window alone while it runs.

Equivalent by hand:

```bash
"F:/Vacancy/Unreal/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "F:/The County Line/CountyLine-Performance/CountyLine.uproject" /Game/Maps/L_JailOffice -game -CLPerfBenchmark -CLPerfExit -windowed -ResX=1920 -ResY=1080 -nosplash -unattended
```

Summarize any capture, at any time:

```bash
"F:/Vacancy/Unreal/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" Scripts/Performance/summarize_benchmark.py "Saved/Performance/<capture>" --json "Saved/Performance/<capture>/summary.json"
```

Output lands in `Saved/Performance/<UTC timestamp>_<platform>/` as `frames.csv` and `run.json`. Nothing is written anywhere else, and `Saved/` is already ignored by git.

### Failing safely

The run aborts, logs one `CL_PERF_ABORT` line and leaves the game playable when: a conflicting mode is present (`-CLSmokeTest`, `-CLTestMission`, `-CLTrialSmoke`, `-CLTrialReview`, `-CLTownReview`, `-CLStreetReview`), the world is not a standalone game world, there is no player controller, the camera cannot spawn, the warm-up or scale arguments are out of range, or the results cannot be written. Input ignoring and the view target are restored on completion, on abort and on an early quit.

## Running on a physical Android device

A packaged APK on the phone is the only Android result. Package and install with `Scripts/Build-Android.ps1` first; nothing here changes signing keys, app ids, release tags, or publishes anything.

```bash
pwsh -File "Scripts/Performance/Get-AndroidCapture.ps1" -EngineRoot "F:/Vacancy/Unreal/UE_5.8" -Arm
```

`-Arm` writes `UECommandLine.txt` into the app's external files directory, launches the app, waits, then pulls captures. Without `-Arm` it only pulls what is already there. It prints the device model and Android version so a capture can be attributed to real hardware.

Manual equivalent:

```bash
adb devices
adb shell getprop ro.product.model
adb shell getprop ro.build.version.release
# Arm the benchmark (package data lives under the app's external files dir):
adb push UECommandLine.txt /sdcard/Android/data/com.amosley0221.countyline/files/UnrealGame/CountyLine/UECommandLine.txt
adb shell monkey -p com.amosley0221.countyline -c android.intent.category.LAUNCHER 1
# ... let the route finish, then:
adb pull /sdcard/Android/data/com.amosley0221.countyline/files/UnrealGame/CountyLine/Saved/Performance "Saved/Performance/Android"
adb shell rm /sdcard/Android/data/com.amosley0221.countyline/files/UnrealGame/CountyLine/UECommandLine.txt
```

`UECommandLine.txt` should contain:

```
../../../CountyLine/CountyLine.uproject /Game/Maps/L_JailOffice -CLPerfBenchmark -CLPerfExit
```

**Delete that file when finished**, or every later launch starts the benchmark instead of the game.

Keep the phone off the charger and the screen awake, and do not switch apps mid-route. Record the device model, Android version and GPU with the numbers; do not compare a Fold8 capture against a PC capture.

### Manual 20-minute heat and resume checklist

No part of this is automated and no thermal value is recorded — the phone's own readings are not read by this harness. Write down what you observe, with times; if something is not measured, say so rather than estimating.

1. Charge to at least 80%, unplug, let the phone sit at room temperature for 10 minutes. Note the ambient conditions.
2. Launch the game normally (no benchmark flag). Note the start time.
3. Run the benchmark capture once, cold. Keep the resulting folder; label it "cold".
4. Play or walk the town continuously for 20 minutes. At 5, 10, 15 and 20 minutes note: whether the phone feels warm, whether motion looks worse than at the start, and any battery percentage drop. These are observations, not measurements.
5. At 20 minutes, run the benchmark capture again without closing the app. Label it "hot". Compare `mean_fps`, `p95_ms` and `frames_over_33_33ms` against the cold capture.
6. Lock the screen for 60 seconds, resume, and confirm the game returns to the same place with input working and no black or frozen view.
7. Press Home, wait 60 seconds, resume, and repeat the check. Then fold/unfold if the device folds, and rotate.
8. Confirm the save still loads after a full app restart, and that no benchmark command line is left on the device.
9. Record the two capture folders, the device model, Android version, and anything you observed. Do not infer a temperature or a sustained frame rate that was not captured.

## Testing

```bash
"F:/Vacancy/Unreal/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" Scripts/Tests/test_performance_summary.py
```

25 tests cover: nearest-rank percentiles including small sets and single samples, odd and even medians, known steady data, uneven frame times, mean FPS versus the average of instantaneous FPS, strict threshold comparisons, warm-up exclusion, a capture with only warm-up rows, per-segment breakdown, missing and present optional metrics, absent optional columns, a missing `phase` column, a truncated final row, non-positive and non-numeric durations, empty and header-only and completely empty files, metadata reading, unreadable metadata, mobile-preview labelling, and the command line including its JSON output and its exit code on a missing capture.

These tests do not need Unreal. They are not registered in `Scripts/Test-Prototype.ps1`, which is outside this task's scope, so **run them explicitly** or add them to the runner separately. No Unreal automation suite was added, for the same reason: that runner fails on any CountyLine suite it does not list.

## Example output

A real rendered capture, 26 September 2026, route version 1, 1280x720 windowed, uncapped, on an RTX 4060 with a Ryzen 5 3600. Development editor build, not a packaged one:

```
  measured frames 8782
  elapsed         42.000 s
  mean FPS        209.09  (frames / elapsed)
  median          4.573 ms
  p95             6.030 ms
  p99             6.934 ms
  min / max       2.063 / 109.239 ms
  over 33.33 ms   23
  over 50 ms      18
  over 100 ms     1
  engine delta    4.783 ms mean
  GPU time        unavailable

  per segment:
    JailOffice            950 frames  158.39 FPS mean
    NorthLane            1423 frames  237.05 FPS mean
  warning: 880 warm-up row(s) excluded
```

The interior is the slowest leg and the open lane the fastest. The 23 frames over 33 ms, one of them over 100 ms, are streaming and shader hitches on a Development build with a partly cold cache; a second run on the same machine is the way to tell a real cost from a first-visit cost. **These are this machine's numbers on this build, not a target.**

`device_make_model` comes from `FPlatformMisc::GetDeviceMakeAndModel()`, which on Windows reports CPU identity rather than a device model; on Android it is the phone's make and model, which is where the field matters.

## Overhead and limitations

- Per frame the harness stores one 40-byte sample and does a few arithmetic operations; files are written once at the end. The measurable cost is a fraction of a millisecond per frame, but it is not zero, and it is inside the numbers reported.
- Timings are game-thread wall clock. With the render thread ahead or behind, these durations describe presented frame pacing, not GPU cost.
- GPU and render-thread timings are unavailable (see above). GPU *identity* comes from `FPlatformMisc::GetPrimaryGPUBrand()`, which may be empty on some platforms.
- `mean_fps` is honest but coarse: always read it with p95, p99 and the threshold counts.
- Editor, PIE and `-nullrhi` runs are not valid performance results. A `-nullrhi` run does exercise the harness end to end, which is how the plumbing was checked without a GPU.
- The route visits exteriors and the jail interior only. Lang's lobby, Bend Lateral and the freight fixture are not covered.
- One capture is one sample of one machine state. Repeat runs, and only compare captures with the same route version, resolution and frame cap.
- Android numbers require a packaged APK on hardware. Nothing in this harness, and no PC run, establishes Android performance.

## What has actually been run

Reported separately, because a script existing is not a measurement:

- **Built**: `CountyLineEditor Win64 Development` with `-NoUBA -NoPCH -NoHotReloadFromIDE`, succeeded, no warnings. The no-PCH build is worth keeping: it caught a missing `GameFramework/Pawn.h` include in this harness that the shared PCH had hidden.
- **Parser tests**: 25 of 25 passing.
- **PC rendered capture**: the example above, produced by `Run-Benchmark.ps1` end to end (launch, capture, summarize).
- **Headless plumbing check**: a `-nullrhi` run produced 4896 samples across all seven segments and wrote both files. Its frame times are meaningless as performance data.
- **Fail-safe check**: `-CLPerfBenchmark -CLSmokeTest` logged `CL_PERF_ABORT` and left the smoke test to finish normally (`CL_SMOKE_RESULT=PASS`).
- **Save isolation check**: no save file was created or touched by any of the runs above.
- **Not run**: anything on a physical Android device. `Get-AndroidCapture.ps1`, the `adb` commands and the 20-minute heat and resume checklist are **untested** — no phone was attached. No thermal or sustained-frame-rate figure exists for any device.

One behaviour worth knowing: on a cold derived-data cache the first run in a fresh worktree ticks only every few seconds while assets compile. The harness detects this and ends with `CL_PERF_SHORT` and a sample count rather than reporting a made-up frame rate. Run it twice and use the second capture.
