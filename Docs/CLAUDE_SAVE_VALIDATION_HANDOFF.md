# Save validation handoff (Claude Code)

Task: `Docs/CLAUDE_SAVE_VALIDATION_PROMPT.md`. Branch `claude/save-validation`, based on `origin/main` at `8f34e36`.

## Changed files

- `Source/CountyLine/Paper/CLSaveValidation.h` / `.cpp` (new): the validator and the validate-then-copy helper.
- `Source/CountyLine/Tests/CLSaveValidationTests.cpp` (new): four automation tests.
- `Scripts/Test-Prototype.ps1`: checks named tests instead of counting successes.
- `Docs/CLAUDE_SAVE_VALIDATION_HANDOFF.md` (this file).

`CLCaseState.h/.cpp`, gameplay, UI, maps, and assets are unchanged.

## Interface

```cpp
#include "Paper/CLSaveValidation.h"

ECLSaveRejection CLSaveValidation::Validate(const USaveGame* Save);
bool CLSaveValidation::CopyIfValid(const USaveGame* Save, FCLReportState& OutReport,
                                   bool& OutTypedCopy, ECLSaveRejection* OutRejection = nullptr);
const TCHAR* CLSaveValidation::RejectionText(ECLSaveRejection Rejection);
```

The acceptance rules match the checks currently inline in `UCLCaseState::Initialize`. They are checked in this order, and the first failure is reported:

1. `NullSave`: the save is null or not a `UCLPrototypeSave`. This is the same as the existing `Cast` then null check.
2. `UnsupportedVersion`: `Version != 1`.
3. `InvalidClosingLine`: `ClosingLine` is outside 0–2.
4. `InvalidStatus`: the status's underlying value is above `Held`. Only Draft, Signed, and Held are accepted.

There are no narrative rules. For example, a signed report that was never read, or a draft that carries carbon data, is still accepted, exactly as today.

`CopyIfValid` writes nothing when a save is rejected. When a save is accepted it assigns the whole `FCLReportState`, so `bBriefedByPruitt`, the field notes, the carbon closing line, and any field added later are carried over without listing them. It also copies `bTypedCopy`.

## Wiring into `UCLCaseState::Initialize` (for Codex)

Add `#include "Paper/CLSaveValidation.h"` to `CLCaseState.cpp`, then replace the cast and the inline `if`:

```cpp
const USaveGame* Save = UGameplayStatics::LoadGameFromSlot(TEXT("CountyLine_JailPrototype"), 0);
if (CLSaveValidation::CopyIfValid(Save, Report, bTypedCopy))
{
    bHasWrittenDate = true;
    SavedReport = Report;
    bSavedTypedCopy = bTypedCopy;
}
```

Nothing else changes: the `-CLSmokeTest` early return and the `DoesSaveGameExist` check stay where they are. A version-2 migration would go into `Validate` and `CopyIfValid`, not into `Initialize`.

## Tests

Every fixture is created in memory (`NewObject`, `SaveGameToMemory`, and `LoadGameFromMemory`). No test loads, writes, or deletes the player's slot.

| Test | Covers |
| --- | --- |
| `CountyLine.Save.Validation.Rejections` | Null saves; versions 0, 2, −1, and the int32 extremes; closing lines −1, 3, 4, and the int32 extremes for every valid status; status values 3, 4, 128, and 255; the order rules are reported in. Every case is checked directly and, where the value can be serialized, after a memory round trip. |
| `CountyLine.Save.Validation.Acceptance` | Default save; Draft, Signed, and Held with each closing line and both copy preferences, with Pruitt's flag both set and unset, directly and after a round trip; states the book cannot produce, which are still accepted. |
| `CountyLine.Save.Validation.CompleteCopy` | A fixture with every reflected field set away from its default, including `bBriefedByPruitt`, is copied field by field. A warning is raised if a new field is left at its default in the fixture. Also checks that the copy is not linked to the save object and that an unbriefed save clears a briefed destination. |
| `CountyLine.Save.Validation.RejectionKeepsState` | A rejected save or a null after an accepted load leaves the report and copy preference unchanged. Validating or copying a save never modifies it. |

Rejections are checked against a destination preset with distinctive values, for both copy preferences. Report comparisons use reflection (`TFieldIterator<FProperty>` plus `CompareScriptStruct`), so fields added later are compared automatically, and a failure names the field.

### Runner

`Scripts/Test-Prototype.ps1` now lists the expected test paths. It fails if an expected test doesn't run or runs twice, if any CountyLine test ends in anything other than `Success`, or if a CountyLine test ran that isn't in the list. **Add new tests to `$expectedTests`.** The runtime smoke check is unchanged.

## Verification

Run on 19 September 2026 in the worktree `F:\The County Line\CountyLine-SaveValidation`, with UE 5.8 at `F:\Vacancy\Unreal\UE_5.8`.

- **Build:** `CountyLineEditor Win64 Development` succeeded with no compile errors or C++ warnings, using a normal precompiled header with no `-NoPCH` or `-NoUBA`. The later incremental rebuild used `-NoHotReloadFromIDE`, because Codex's `CountyLine-Office` game window was running with Live Coding. That lock is engine-wide, but the worktree writes to its own `Binaries`, so the running game was not affected or stopped.
- **Automation:** `Scripts/Test-Prototype.ps1 -EngineRoot F:\Vacancy\Unreal\UE_5.8` exited 0 and printed "All 7 expected CountyLine automation tests succeeded."
  - All 7 tests reported `Result={Success}`: the three existing tests and the four new ones.
  - The log had 0 automation warnings or errors, so the fixture-coverage check raised no warnings.
- **Runtime smoke:** `CL_SMOKE_RESULT=PASS`, with 29 PASS lines and 0 FAIL lines.
- **Mutation check:** I temporarily changed `CopyIfValid` to flip the copy preference when it rejects a save and to clear `bBriefedByPruitt` when it copies. `CompleteCopy`, `Rejections`, and `RejectionKeepsState` failed with 108 errors naming the field or state involved. `Acceptance` was then given briefed fixtures so it catches the second break too. The helper was restored before the final run above.
- **Runner check:** I fed the runner's parsing logic doctored copies of the log. It passed the real log and failed with a specific message for a failed test, a missing test, a test not in the list, and a test that ran twice.
- The engine rewrites `Config/DefaultEngine.ini` with an Android File Server `SecurityToken` on every run. That change was reverted and is not committed.

Build, automation, and smoke results are reported separately. No editor-play or packaged-build check was done.

## Limitations

- The `NullSave` case where a save has the wrong class isn't exercised. `USaveGame` is abstract, and creating a second save class just for tests would need reflected test code outside the files this task may change. The branch is the same `Cast` that `Initialize` uses today.
- The helper isn't wired into `Initialize` yet (see above), so the automated smoke test doesn't exercise it.
- Invalid status values aren't serialized through memory, because Unreal may log about unknown enum values. They are validated directly.
