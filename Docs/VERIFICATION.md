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

## Claude test integration — 19 September 2026

Reviewed and integrated Claude's completed `CLPrototypeTests.cpp` changes from `claude/report-save-tests`. The suite checks all 24 fact/closing-line/submission combinations, rejected submissions without mutation, locked carbon copies, draft round trips, and byte-identical saves after rejected attempts. Save/load tests operate in memory and do not touch the player's save slot.

Integration adds field-by-field comparison of `bBriefedByPruitt` and both introduction states to the submitted-save matrix: 96 combinations across facts, closing lines, outcome, copy preference, and introduction state. The existing three automation test names are preserved for the test runner.

The combined editor target built successfully with `-NoPCH -NoUBA`, confirming the earlier missing-include issue is resolved. The inline version/range checks in `UCLCaseState::Initialize` still lack direct malformed-save tests; this integration does not claim that coverage. Generated precompiled-header backups in the original checkout are unchanged and remain excluded from Git.

Combined verification: Scripts/Test-Prototype.ps1 exited 0; all three expanded automation suites and all 22 runtime smoke checks passed against the merged office/controller build.

## Save-validation integration

Reviewed Claude's commit `f4f456ebaff281276c2901527ab9a020e41367a9` and connected `CLSaveValidation::CopyIfValid` to `UCLCaseState::Initialize`. The loader retains its smoke-test bypass and missing-slot check. Only an accepted save marks the state saved and updates its saved snapshot. Whole-report copying preserves field notes, Pruitt's introduction and carbon wording. Rejected saves leave the destination unchanged; acceptance rules remain identical to the previous inline checks.

The runner now requires all seven named automation suites, including the four new validation suites. Validation fixtures use memory-only serialization; integration verification does not overwrite the player's save slot. The runtime smoke bypasses disk loading, so it checks gameplay regressions rather than exercising the disk-load path. Packaged-build and wrong-save-class fixture coverage remain outside this integration.

Integrated verification (19 September 2026): UE 5.8.2 CountyLineEditor Win64 Development built successfully with `-NoPCH -NoUBA -NoHotReloadFromIDE`. `Scripts/Test-Prototype.ps1` exited 0: seven named automation suites succeeded and all 29 runtime smoke assertions passed. Existing playable checkout and desktop shortcut use the rebuilt module.

## Bend Lateral visual pass — 19 September 2026

UE 5.8.2 CountyLineEditor Win64 Development built successfully without precompiled headers. Scripts/Test-Prototype.ps1 exited 0: all seven named automation suites and all 34 runtime smoke assertions passed. New checks cover Reed's imported scale and material assignments, both period character meshes, six scenery meshes and channel alignment with gameplay coordinates.

The running game was inspected after the final asset import. Reed and Salazar display their authored clothing colors and animate on the existing skeleton. Office-to-field travel, Salazar's contextual prompt/conversation, and the bottle's visible placement/prompt/examination were checked interactively. Repeatable development-console positions were used for the witness and bottle checks. No player save was written. Return travel and field-note preservation passed the runtime smoke test.

This remains an initial stylized art pass: simplified faces, rounded shoulder joins, basic garment deformation, static water and placeholder foliage. Pruitt still uses the mannequin. This verification does not establish production animation quality, physical-controller operation or packaged-build compatibility. Editable art sources and reproduction steps are recorded in Docs/VISUAL_PASS.md.

## Field inspection, motion and sound — 19 September 2026

The UE 5.8.2 editor target builds successfully with -NoPCH -NoUBA. Scripts/Test-Prototype.ps1 exited 0: all seven automation suites and all 43 runtime smoke assertions passed. New checks cover posed character scale, movement-triggered footsteps, three looping ambience assets, channel spatialization, the live witness gesture, controller orbit/zoom, cancel restoring the pawn/camera/input without evidence, and outdoor audio deactivation on return.

The interactive check covered the corrected idle, Salazar's moving gesture, bottle and bank close-ups, Q orbit, R zoom and Escape restoring third person. The notebook remains beside the scene and recording evidence is still explicit. A first animation import introduced a 100x root scale; the importer now normalizes the original in-place clips and the smoke test explicitly evaluates bones under -nullrhi before checking posed height.

Audio files were checked for valid PCM data and headroom; Unreal initialized the hardware audio mixer successfully. Automated checks validate assets, activation state and attenuation configuration, not perceived sound quality. A listening pass on the player's speakers/headphones and a physical-controller playtest remain useful. No player save was written. The scene still uses prototype art and synthesized sound, with no voice acting or lip sync. Sources and controls are documented in Docs/FIELD_IMMERSION.md.

## Evidence follow-up and consequences — 19 September 2026

The UE 5.8.2 editor target built successfully with -NoPCH -NoUBA. Scripts/Test-Prototype.ps1 exited 0: all eight named automation suites and all 51 runtime smoke assertions passed. The new suite covers each lead and its prerequisites, cancellation/repeat protection, pending and completed lead persistence, and all seven nonempty finding combinations across both original report dispositions and both follow-up outcomes. Malformed follow-up records are rejected without mutating the destination. Save fixtures use memory serialization only.

Simulated controller input selects a lead in Cases, cancels and reopens its field inspection, records the finding, opens the consequence preview, and confirms filing at the desk. The checks also verify rejection away from the desk, unchanged original carbon, unsaved-state detection, and safe focus after filing. This does not replace a physical-controller playtest.

At 1440 x 900, inspected the filed inquiry and its complete prose in the scrollable Cases page, the changed Ledger, and Pruitt's inquiry response/save reminder. This used -CLSmokeTest -CLSmokeKeepOpen, which retains the successful smoke scenario for inspection while preventing player-slot loading or writes. No player save was changed. Follow-up decisions currently produce persistent records, Ledger prose, and dialogue; offscreen inquiry simulation, medical findings, and a full hearing remain future work. See Docs/EVIDENCE_FOLLOWUP.md for the playable sequence and scope.
