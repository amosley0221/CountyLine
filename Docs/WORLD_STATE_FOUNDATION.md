# World state foundation (Claude Code)

Branch `claude/world-state-foundation`, based on `origin/main` at `1c3ec87`.

Persistent storage for future open-world exploration: which locations the player has discovered, and the last place they stood safely. This is storage and validation only. No terrain, map, streaming, fast travel, or UI is included, and nothing reads these fields for spawning or travel yet.

## Interface

`Source/CountyLine/World/CLWorldState.h`:

```cpp
USTRUCT() struct FCLWorldState
{
    TArray<FName> DiscoveredLocations;   // unique, in discovery order
    FName LastSafeLocation;              // NAME_None until recorded
    FTransform LastSafeTransform;        // identity until recorded

    static bool IsValidLocationId(FName Id);
    static bool IsFiniteTransform(const FTransform& Transform);

    bool DiscoverLocation(FName Id);        // false for an invalid or already known id
    bool IsLocationDiscovered(FName Id) const;
    int32 NumDiscoveredLocations() const;
    bool SetLastSafePosition(FName Id, const FTransform& Transform);
    bool HasLastSafePosition() const;
    bool IsValidState() const;
    void Reset();
};
```

**Location ids** are 1–64 characters, starting with a letter, then letters, digits, or underscores. `NAME_None`, empty text, spaces, and punctuation are rejected, so ids stay usable as map, table, and telemetry keys. Ids are `FName`s, so comparison ignores case: `JailOffice` and `jailoffice` are the same location.

**Transforms** are rejected when any translation, rotation, or scale component is NaN or infinite, or when the rotation quaternion is not normalized.

Every operation is all-or-nothing: a rejected call returns false and changes nothing.

`SetLastSafePosition` does not discover the location. Whoever records a safe position decides whether standing somewhere also counts as discovering it.

## Save integration

- `UCLPrototypeSave` gained `UPROPERTY() FCLWorldState World`.
- `UCLCaseState` gained a matching `World` member, loaded in `Initialize` and written in `WriteDate`.
- `CLSaveValidation` gained `ECLSaveRejection::InvalidWorldState` and a fourth-argument overload:
  `CopyIfValid(Save, OutReport, OutTypedCopy, OutWorld, OutRejection = nullptr)`.
  The existing three-argument overload is untouched, so current callers keep working.
- A save is rejected when its world state has malformed or duplicate ids, a non-finite last safe transform, or a transform set with no location. Report, follow-up, and version rules are checked first and still report first.

Save version stays **1**. World state is a new tagged property, so older saves simply lack it and load with an empty, valid world state, the same mechanism the follow-up fields rely on.

## Not wired up

- Nothing calls `DiscoverLocation` or `SetLastSafePosition` during play. Travel and spawning are unchanged.
- `IsCurrentStateSaved` still compares only the report and copy preference. Once something mutates world state at runtime, add `World` there, or the "unsaved" notice will miss world-state changes.
- No gameplay consequence, UI, or authored location list is included. Location ids are caller-defined; there is no registry of known places yet.

## Verification (19 September 2026, UE 5.8 at `F:\Vacancy\Unreal\UE_5.8`)

- Build: `CountyLineEditor Win64 Development` succeeded, no errors or C++ warnings. `-NoHotReloadFromIDE` was used because a `CountyLine-Office` game window was running with Live Coding; that lock is engine-wide and this worktree writes to its own `Binaries`. The other checkout was not touched or stopped.
- `Scripts/Test-Prototype.ps1 -EngineRoot F:\Vacancy\Unreal\UE_5.8` exited 0: "All 10 expected CountyLine automation tests succeeded." 10 successes, 0 non-successes, 0 automation warnings or errors.
- Runtime smoke: `CL_SMOKE_RESULT=PASS`, 51 passes, 0 failures.
- Mutation check: allowing duplicate discoveries, skipping the finite-transform check, and clearing the last safe location during a copy each failed the new tests (53 errors, both suites failing). The code was restored and the results above are from the restored build.
- All test fixtures are memory-only. No test loads, writes, or deletes `CountyLine_JailPrototype`.

## Compatibility limitations

- Old-save behavior is covered by a save whose world state was never set, which is what an older save deserializes to. Genuine pre-change bytes cannot be produced in a test, because the save class name is written into the data; this rests on Unreal's tagged property serialization, as the follow-up fields do.
- Saves written after this change carry world state and still read as version 1. An older build reading such a save ignores the unknown property, so round-tripping a save through an older build silently drops world state.
- Case-insensitive ids mean `BendLateral` and `bendlateral` cannot be different locations. Pick one spelling per place.
- The rotation must be normalized, so a caller passing a raw interpolated quaternion should normalize it first.
- Discovery order is preserved and the list is unbounded; there is no cap, pruning, or grouping by region yet.
