# Prototype verification — 18 September 2026

## Build and content

- Unreal Engine 5.8.2, changelist 56702186.
- `CountyLineEditor Win64 Development`: succeeded after the final UI layout change, exit code 0.
- Toolchain: Visual Studio 14.44.35228 and Windows SDK 10.0.22621.0.
- Non-blocking environment warning on full builds: Visual Studio SDK unavailable for editor integration. C++ compilation and linking succeeded.
- Unreal Python generated `/Game/Maps/L_JailOffice` and ten prototype materials.
- Content validation found **114 mesh components**, each with a mesh and material.
- Template blend-space inspection confirmed speed is axis 0; character movement supplies planar speed on that axis. Animation starts explicitly at BeginPlay.
- Git LFS is initialized locally for `.uasset`, `.umap`, and `.fbx`. Generated output and saves are ignored.
- Original twelve reference files were verified against the source ZIP during setup and remain unmodified.

## Automated checks

`Scripts/Test-Prototype.ps1` completed successfully against the gameplay/focus/save-feedback build. The subsequent change only adjusted pause scrolling and fact-label width; that change was rebuilt and visually reviewed.

Three Unreal automation tests passed:

1. `CountyLine.Interaction.RangeAndView`: reach, cone, behind-camera rejection, and pawn-based reach independent of camera offset.
2. `CountyLine.Report.SubmissionAndCarbon`: unread/invalid rejection, SIGN and HOLD consequences, included/omitted facts, and duplicate-submission prevention.
3. `CountyLine.Report.SaveRoundTrip`: serialized status, omitted fact, closing line, and copy preference survive save/load.

Runtime smoke test passed ten assertions: controller/pawn creation, movement advancing the character, mesh and animation instance loading, grounded character, wall blocking, near-desk acceptance, paused/input-owned book, restored gameplay after closing, and distant save-station rejection. Smoke mode bypasses the user's save slot.

Logs: `Saved/Logs/PrototypeTests.log`, `PrototypeSmoke.log`, and `PrototypeContent.log`.

## Interactive game-window checks

- Inspected the furnished office and animated mannequin.
- Observed the desk prompt and used E to read the report, then entered the County Book.
- Omitted the bottle, selected the further-inquiry closing line, chose HOLD, and observed the held stamp and locked controls.
- Switched typed copy / handwriting, saved at the desk, quit, and relaunched. The held status, omitted bottle, closing line, and handwriting preference survived.
- Confirmed saving is disabled away from the desk.
- Verified arrow-key focus navigation and Escape/Tab returning control to gameplay.
- Tested Start Again without overwriting the saved date. The QA-created save was moved to `Saved/Verification/HeldReport_Playtest.sav` so the delivered prototype starts fresh.

The detailed desk UI test used Unreal's development console to set a repeatable position. Character movement and wall blocking were tested separately in the runtime smoke test.

## Limits

This is an uncooked prototype running with the installed editor runtime. No packaged standalone Windows build, physical controller test, performance certification, production Reed rig, or retargeted user FBX set has been completed. Map and Payroll pages identify unfinished systems. The initial prototype and source/reference backups were subsequently pushed to GitHub on 19 September.

## Office conversation and controller pass — 19 September 2026

Implemented in an isolated `feature/office-conversation-controller` worktree, leaving Claude's `Source/CountyLine/Tests/CLPrototypeTests.cpp` edits untouched. Sam Reed remains the player; Deputy Pruitt introduces the report through prototype-authored text. The existing map receives the deputy, idle animation, collision, reading light, book, ink bottle, and other desk props through native components.

The editor target builds successfully with UE 5.8.2 (`-NoPCH -NoUBA` on this machine). Explicit mesh and input-component includes also allow compilation without relying on precompiled headers. The three existing automation tests passed. Runtime verification includes simulated controller events routed through Slate; this does not substitute for testing an actual connected controller. Xbox-style labels are used, with no claim of native PlayStation glyphs or controller rumble.

Visually checked the office, the Pruitt interaction prompt, both conversation pages, and visible keyboard focus at 1440 x 900. Character art and speech remain temporary mannequin/text presentation.

The expanded runtime smoke test passed all 22 checks, including Pruitt's mesh/idle, conversation cancel/advance/finish, report entry, D-pad fact selection, left-stick focus, closing-line focus retention, HOLD with an omitted fact, shoulder-page navigation, and B returning to gameplay. A first run caught Slate consuming analog events on child controls; custom book controls now let the book handle those events. Simulated events use Slate's normal input routing.

Interactive checks also covered report entry, focus wrapping, automatic scrolling to HOLD, disabled choices after submission, writing the date, and marking the state unsaved after changing copy preference. Superseded save notices are cleared when an editable choice changes.

The final notice-cleanup build passed the same 22 runtime checks. Relaunching loaded the held report and displayed the saved-completion objective. The QA save was archived outside the active slot under Saved/Verification so the delivered office starts fresh.
