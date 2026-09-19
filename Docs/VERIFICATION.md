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

This is an uncooked prototype running with the installed editor runtime. No packaged standalone Windows build, physical controller test, performance certification, production Reed rig, or retargeted user FBX set has been completed. Map and Payroll pages identify unfinished systems. Changes are local; no GitHub push was made.
