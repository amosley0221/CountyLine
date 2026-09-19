# Claude Code: save validation

Work on The County Line while Codex builds Bend Lateral.

Repository: https://github.com/amosley0221/CountyLine
Start from origin/main at adb29eb or a newer descendant, in a separate worktree on claude/save-validation. Do not switch or edit Codex's CountyLine-Office checkout.

Own only:
- New Source/CountyLine/Paper/CLSaveValidation.h and .cpp
- New Source/CountyLine/Tests/CLSaveValidationTests.cpp
- Scripts/Test-Prototype.ps1, only as needed to support additional tests
- Docs/CLAUDE_SAVE_VALIDATION_HANDOFF.md

Create a reusable validator and a validate-then-copy helper for UCLPrototypeSave. Preserve the current acceptance rules: non-null save, Version == 1, closing line 0-2, and report status Draft/Signed/Held. Do not invent stricter narrative rules. Rejection must leave all destination state unchanged. Successful copying must preserve the complete report, including Pruitt's introduction flag and any fields Codex adds later. Copy the struct rather than enumerating individual fields.

Test null saves, unsupported versions, invalid closing lines/status values, valid drafts and submissions, copy preference, complete-state preservation, and rejection without mutation. Use memory-only fixtures; never load, overwrite, or delete the player's save slot.

Do not edit CLCaseState.h/.cpp, gameplay, UI, maps, or assets. Document the small change needed to wire your helper into UCLCaseState::Initialize; Codex will perform that integration after both tasks finish.

Keep existing tests intact. If updating the runner, check expected test names and failures rather than merely accepting any number of successes. Preserve the runtime smoke check.

Build with UE 5.8 at F:\Vacancy\Unreal\UE_5.8. Use -NoPCH -NoUBA if necessary. Do not stop unrelated Unreal/build processes. Exclude generated output and Android File Server SecurityToken changes from commits.

Commit and push your task branch only, not main. Return the commit hash, files changed, exact verification results, and remaining limitations.
