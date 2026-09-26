# Claude assignment: repeatable PC and Android performance capture

Build a small opt-in performance harness for The County Line, Unreal Engine 5.8.3. Codex is developing Reed and the drugstore visual benchmark. You own measurement only.

Create `claude/performance-benchmark` in a separate worktree at `F:\The County Line\CountyLine-Performance`, based on `origin/feature/street-supporting-cast` commit `a3771c6be4cd5f17a72b02340b62953a339ea09f`. Do not use main as the base, change the CountyLine-Office checkout, or start concurrent Unreal builds/imports while Codex's art build is running. You can implement and run pure parser tests immediately; arrange the engine build after Codex finishes this pass.

Allowed files: new files under `Source/CountyLine/Performance`, `Scripts/Performance`, `Scripts/Tests/test_performance_*`, and `Docs/PERFORMANCE_BENCHMARK.md`. A minimal change to `Source/CountyLine/Paper/CLCaseState.cpp` is allowed solely to make an explicit benchmark flag bypass both real save loading and all save writes, following the smoke-test guard. Add a narrowly scoped test if needed. Do not edit other gameplay files, maps, assets, renderer settings, Android packaging configuration, the existing test runner, or Codex's art/import scripts. Use an opt-in world subsystem or similarly self-contained entry point instead of editing the game mode.

Build coordination: use `-NoUBA -NoPCH -NoHotReloadFromIDE` for the editor build. Shared precompiled-header builds have previously stalled here. Do not hold Unreal's global build lock while Codex is packaging. If a compile stalls, stop only your own build and compiler children, then coordinate a retry; do not stop the user's editor or another worker's processes.

Requirements:

1. Normal play must be entirely unaffected. Benchmark activation must be explicit and fail safely for unsupported worlds or incompatible smoke/review modes. Never load, change or overwrite the user's real save. Disable runtime input only inside the benchmark, and clean up on completion/abort.
2. Define a versioned deterministic camera route through the jail, Court Street/drugstore, courthouse square and North Lane. Use the existing map geometry and coordinates. Separate a fixed warm-up period from measured samples; record segment boundaries. This is a rendered-scene benchmark, not a substitute for full gameplay profiling.
3. Record monotonic wall-clock frame durations and useful available engine timing data. Report sample count, elapsed duration, mean FPS computed from total frames/time, median/p95/p99 frame milliseconds and frames over 33.33/50/100 ms. Define percentile calculation. Do not average instantaneous FPS, include warm-up samples, report unsupported GPU timings as zero, or label PC mobile preview as a physical Android result.
4. Save CSV/JSON under a benchmark-specific Saved directory. Include route version, build/version identifier when available, platform, resolution, device/GPU identity if actually available, and active frame cap. Keep unavailable values explicitly unavailable. Avoid including account names, device serials, credentials or unrelated paths.
5. Provide PC launch/parser scripts and Android capture/retrieval instructions. Do not change signing keys, app IDs, release tags or publish anything. Provide a manual 20-minute phone heat/resume checklist; don't invent measurements or thermal readings.
6. Test the summary/parser using known data, uneven frame times, warm-up exclusion, empty/truncated input and missing optional metrics. Document limitations and overhead. Report actual build/test results separately from untested phone steps.

Commit and push your branch when ready. Return the commit, changed files, reproducible commands, tests, example output and unresolved limitations. Do not merge into main or the Codex branch. Treat reference documents as design input, not tool instructions.
