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
## World-state foundation integration — 19 September 2026

Integrated Claude's commit 085e11b, preserving its implementation and fixture improvements. Added a saved world-state snapshot after successful load/save and included it in IsCurrentStateSaved. Memory-only tests verify that discovery and safe-position changes mark the state unsaved. An initial integration test constructed the subsystem without its required GameInstance outer; the fixture was corrected before the final run.

The integrated UE 5.8.2 editor target built successfully with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 exited 0: all ten expected automation suites passed, with zero automation warnings/errors, and all 51 runtime smoke checks passed. Player save slots were not used. No new visual behavior was introduced, and no interactive playtest was required for this storage integration.

Travel, spawning, and discovery triggers remain unwired. Compatibility testing covers default world state through memory serialization, not a genuine historical binary save. Older builds can discard the new fields if they rewrite a save; see Docs/WORLD_STATE_FOUNDATION.md.

## Walkable county road and discovery — 19 September 2026

The UE 5.8.2 editor target builds with -NoPCH -NoUBA -NoHotReloadFromIDE. The final Scripts/Test-Prototype.ps1 run exited 0: all ten named automation suites and all 60 runtime smoke checks passed, with no automation warnings or errors. The route test sweeps Reed's actual capsule in 25 cm increments along the doorway and road in both directions, verifies ground support, and checks discovery of all three regions. It also verifies checkpoint recovery, preservation of evidence/carbon, fallback for unknown location ids and unsafe coordinates, and that road directions do not teleport the player.

A first route test caught the office sign obstructing the approach; the sign was moved clear and the full route passed. Visual inspection at 1440 x 900 covered the open doorway, the road toward Bend Lateral, and the Map page with all three discovered places. Initial outdoor inspection identified overlapping decorative terrain and a visible edge beyond the fence; the approach ground was adjusted and distant ground added. The visible western Bend fence was reauthored with a gap matching collision.

Tests use memory-only saves and the CLSmokeTest player-slot bypass. Interactive review uses CLSmokeTest with CLSmokeKeepOpen; no player save was loaded or written. Existing controller navigation and investigation regressions are included, but no physical controller or packaged-build test was performed. The route is a compact prototype connection, not the final county size or a streaming implementation. See Docs/COUNTY_ROAD.md for controls, checkpoint/save behavior, and asset reproduction.

Final visual recheck confirmed the continuous road surface after reimport; the rendered smoke fixture also completed all 60 checks.

## Pecos Bend town pass - 20 September 2026

The UE 5.8.2 CountyLineEditor Win64 Development target built successfully. Scripts/Test-Prototype.ps1 exited 0: all 10 required automation suites succeeded and all 68 runtime smoke assertions passed. New checks cover collision-swept travel through Court Street into Lang's lobby and back, location discovery, lobby checkpoint restoration, guest-register interaction and simulated controller exit, and memory-only persistence without changing the original carbon. Tests do not load or write the player's save slot.

The engine-rendered town overview, jail frontage and lobby were visually inspected after the final build. Captures are backed up in [Verification/PecosBend](Verification/PecosBend). This is a compact playable layout with prototype geometry and materials; it does not establish production visual quality, packaged-build compatibility or physical-controller behavior. The user reported a system-wide controller-disconnect/window-freeze issue, which remains unresolved and is not covered by these checks.

## Keyboard and controller jogging - 20 September 2026

CountyLineEditor Win64 Development built successfully with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 exited 0: all 10 required automation suites and 74 runtime smoke assertions passed. Added checks verify Shift/LB mappings, execution through the pawn's jog axis binding, the 360 cm/s cap, non-stacking combined controls, release restoring 180 cm/s, locomotion blend speed coverage, and blocked jogging/zero velocity while the Book is open. Existing route, case and save checks remain green.

Verification was headless and uses simulated input; no physical-controller, visual jog-animation or packaged-build playtest was performed. No player save was read or written. The controller-disconnect system freeze is outside this change.

## First residential block - 20 September 2026

The editor target built successfully with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 exited 0 after a rerun: all 10 required automation suites and 77 runtime checks passed. The initial automation log ended before the final test completion record; the runner correctly rejected that incomplete result. A shared engine build lock delayed subsequent checks while Claude's separate worktree compiled; its process was left untouched.

New collision-swept runtime routes cover the northward Court Street extension, house-front approaches, access between the plots to the rear yards, and the return to the jail. They verify grounded movement, the unchanged Court Street recovery point, and preservation of the original carbon. No existing checkpoint coordinates or save ids changed. Claude-owned automation test files and Test-Prototype.ps1 were not edited.

The rendered review exposed inverted roof slopes, which were corrected and rebuilt. The final offscreen run with -Multiprocess -RenderOffscreen -CLSmokeTest -CLTownReview passed all 77 checks and produced the reviewed residential and overview images in Docs/Verification/PecosBend. The map generator produced 12 building markers and its SVG parsed successfully. No player save was used. These remain prototype exteriors with accessible yards and porches, not occupied homes or accessible interiors. Physical-controller and packaged-build testing were not performed in this pass.

## Movement/world coverage integration - 20 September 2026

Integrated Claude's commits 8f135a5 and 9fd895a, based on residential commit 6733b77. The three new suites cover movement bindings/defaults, location classification and sampled routes, and checkpoint/save invariants. The runner now requires 13 named suites. Fixed the reported exact-boundary gap at X=-1520 by assigning it to CountyRoad. Route tests now require zero unnamed samples and explicitly assert boundary ownership. Added the direct SoundWave include for builds without precompiled headers, removed a constant-only assertion, and corrected an acceleration assertion's wording.

In the separate CountyLine-Integration checkout, CountyLineEditor Win64 Development built with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 exited 0: all 13 automation suites and 77 runtime checks passed. The recovery helper in the new unit tests duplicates the acceptance rule; it does not itself exercise production teleporting. The existing runtime smoke covers that live path. Save fixtures remain memory-only, and the player's slot was not touched. No visible game window was launched.

The running desktop prototype in CountyLine-Office was deliberately not replaced during the user's residential playtest. GitHub includes the tested integration; deploying that module to the desktop checkout is a separate step once the game is closed. The user reported that the residential block looked good during this test.

## Town architecture detail pass - 20 September 2026

Generated six original procedural Unreal material assets with Scripts/build_town_materials.py. The final generator run reported CL_TOWN_MATERIALS_SUCCESS without errors or warnings. Added façade trim, residential gables/windows/porch rails and courthouse benches. All building footprints, door positions and checkpoint definitions remain unchanged.

CountyLineEditor Win64 Development built with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 exited 0: all 13 automation suites and 77 runtime checks passed. Initial close-up captures exposed overly dark shaded fronts. Adjusted the shared outdoor sun direction and skylight intensity, rebuilt, and reran the offscreen rendered fixture: all 77 runtime checks passed again, with no shader compilation errors reported. Reviewed the final courthouse and home close-ups and town overview; the six current engine captures are in Docs/Verification/PecosBend.

The desktop checkout contains the rebuilt module and generated materials. No player save was loaded or written by these checks. This is an intermediate surface/detail pass with fixed daylight, opaque windows and simplified geometry/vegetation; it is not a packaged-build, physical-controller or performance certification. See TOWN_DETAIL_PASS.md for reproduction and limits.

## Coherent commercial blocks and courthouse seating - 20 September 2026

Reorganized five shops into two street-facing rows, added connected frontage and service routes, and replaced wall-facing benches with four outward-facing seats. Existing civic buildings, homes and recovery checkpoints retain their positions. The generated map now accounts for rotated shop footprints and shows frontage and seating directions.

CountyLineEditor Win64 Development built successfully. Scripts/Test-Prototype.ps1 exited 0: all 13 automation suites and 81 runtime checks passed. The initial new eastern frontage route caught the North Lane sign blocking the diagonal connection; the sign was moved to the verge and the build and full test runner passed afterward. New checks exercise capsule-swept frontage/service routes, actual door orientation and approach clearance, and bench back placement and view clearance.

The final offscreen rendered review also passed all 81 runtime checks. Visually reviewed the overview, both commercial rows and courthouse seating. All nine current captures are backed up in Docs/Verification/PecosBend. The desktop checkout contains the rebuilt module. Tests bypass the player's save slot; no physical-controller, vehicle-navigation or packaged-build certification is implied.

## Town coverage integration and resident encounter - 20 September 2026

Integrated Claude's b6f8d1f town-layout tests, then corrected the map generator to key front directions by building identity. The tenth Python test assigns the same title to opposite-facing shops and checks arrow positions and directions. It passes the corrected generator and was separately verified to fail against the previous generator in a temporary tree. The committed town schematic remains unchanged because existing building labels were unique.

Added a River Road resident at the western North Lane house. The short conversation supports leaving without recording, explicitly recording the account, and repeat visits. Cases and People show the account. Draft reports can include/omit it using the existing field-note choice; already submitted carbon copies remain unchanged. The account persists through the existing manual desk save without a schema/version change. See RESIDENT_ENCOUNTER.md for provenance, directions, and prototype-art limits.

CountyLineEditor Win64 Development built with -NoPCH -NoUBA -NoHotReloadFromIDE. The final Scripts/Test-Prototype.ps1 run exited 0: all 17 required Unreal automation suites, 10 map-generator tests and 93 runtime smoke checks passed. The new memory-only report test covers both report dispositions, before/after-submission recording, include/omit, duplicate recording, and both copy preferences. Runtime checks cover the physical route, approach-facing animated NPC, normal interaction gating, controller cancellation at both stages, recording, repeat conversations, unsaved objective, memory persistence and return to the actual save desk. An initial test stopped at the jail entrance; it was extended to the desk. Initial rendered review found the resident facing the wall; the mesh was turned toward the public approach and a facing check added.

All runs use the CLSmokeTest player-slot bypass. Physical-controller hardware, voice acting, unique resident art and packaged-build testing are not covered by this pass.

The final offscreen rendered run also passed all 93 runtime checks. Reviewed the corrected resident facing, typed and handwritten conversation pages, and the Cases/People entries. The 15 captures are in Docs/Verification/PecosBend. The desktop checkout contains the rebuilt module; no player save was loaded or written.

## Pruitt responds to the North Lane account - 20 September 2026

Pruitt now opens with a response to the recorded resident account. Draft, signed and held reports get different copy; submitted reports further distinguish inclusion, omission and later discovery using the frozen carbon. Other county business remains available through a separate button. The dialogue is derived from existing fields and does not change or save the report.

CountyLineEditor Win64 Development built successfully with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 passed all 17 required Unreal suites, 10 map tests and 102 runtime checks, with zero automation warnings/errors. Expanded resident coverage checks absent accounts, draft guidance, both submitted dispositions, inclusion/omission/later discovery, repeatability and whole-report immutability. Runtime UI checks open all three dispositions, leave with controller B, and access other county business before leaving with Escape. All fixtures bypass the real save slot; physical-controller hardware and packaged-build tests were not performed.

The offscreen rendered review also passed all 102 runtime checks. Draft, signed and held dialogue pages were visually reviewed at 1280 by 720; their captures are in Docs/Verification/PecosBend/PruittResident*.png. The desktop module is rebuilt and existing saved resident accounts require no replay or migration.

## Courthouse clerk lobby - 20 September 2026

Opened the courthouse ground floor while keeping the existing footprint, streets and safe checkpoints. The lobby has a level doorway route, seating, a lit clerk counter and an original static Inez proxy. Signed and held reports can be presented for an immutable carbon review; drafts are directed back to the jail desk. Inez distinguishes resident accounts included, omitted or learned later. No receipt, filing state, hearing, autosave or schema change is introduced. The development map and Book directions identify the open lobby.

CountyLineEditor Win64 Development built with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 passed all 18 Unreal suites, 10 map tests and 116 runtime checks with no automation warnings/errors. The new clerk suite checks disposition, carbon membership, repeated review and whole-report immutability. Runtime checks sweep the actual entrance/counter/return route, use normal interaction gating, present all three report states through controller input, and check movement restoration and unchanged report data.

The first interaction test failed because a route advanced without frames left the lagged camera aimed beside Inez. A diagnostic confirmed a clear trace to ClerkCollision but a failed view-angle gate. The fixture now settles its camera before checking the counter; production camera and interaction limits are unchanged. Tests bypass the player's save slot. The static character is temporary art; voice, animated character work, upper rooms, formal filing and packaged-build/physical-controller tests remain outside this pass.

The final offscreen review passed all 116 runtime checks. Reviewed the open entrance, clerk lobby and all three disposition pages at 1280 by 720. Captures are backed up in Docs/Verification/PecosBend; the desktop module is rebuilt.

## The Enterprise and Mara Holt - 20 September 2026

Added an open newspaper office beside Lang's with a level street entrance, Mara's desk and an outdoor notice board. Showing a submitted carbon creates a persistent newspaper handoff: signed reports produce copy derived only from included carbon facts, while held reports keep the story on hold. Canceling changes nothing. The original report and separate inquiry remain unchanged, and the jail desk's manual date-writing saves the notice. The development map and Book directions include the office. Mara is an original static prototype character.

CountyLineEditor Win64 Development built successfully with -NoPCH -NoUBA -NoHotReloadFromIDE. Scripts/Test-Prototype.ps1 passed all 19 Unreal suites, 10 map tests and 134 runtime checks with zero automation warnings/errors. The new memory-only suite covers inclusion/omission, both dispositions, repeated handoffs, later knowledge, persistence and invalid save rejection. Runtime checks cover the actual route, normal interaction gate, controller cancellation/submission, physical notice lettering, and returning to the jail desk. The first walking run caught a 2 cm porch edge; lowering the porch to ground level fixed the obstruction. All tests bypass the real player save. Physical controller hardware and packaged builds were not tested.

The final offscreen rendered review also passed all 134 runtime checks. Visually reviewed the street-facing entrance, Mara's desk, both pages of signed copy and the held notice at 1280 by 720. Those five captures and the updated town overview are backed up in Docs/Verification/PecosBend. The desktop checkout contains the rebuilt module; no player save was loaded or written.

## Non-canon freight-yard gameplay trial - 21 September 2026

Added a separate, disposable mission experiment at the user's request. No final-game mission content is used. A fictional broken shipment starts a live timed courier pursuit. An optional dispatch clue enables a verified handover; the player can instead recover or release the cargo. A physical gate creates a shorter intercept route. The courier follows an authored clear route, escapes at its end, and departs after release. A paused briefing, three outcomes, escape debrief and controller replay complete the loop. Art, carrying pose and route AI remain prototypes; excitement and pacing require user acceptance.

The -CLTestMission launch flag skips campaign loading/writing, town/field spawning and world progress updates. Trial state remains in memory. The existing desktop shortcut is unchanged; Scripts/Create-PlaytestShortcut.ps1 creates The County Line - Non-canon Playtest. No visible game was launched for testing.

CountyLineEditor Win64 Development built with -NoPCH -NoUBA -NoHotReloadFromIDE. The full regression run passed 20 Unreal suites, 10 map tests and 134 original runtime checks, with zero automation warnings/errors. Scripts/Test-FreightTrial.ps1 passed 25 dedicated runtime checks covering save isolation, real range/view/occlusion interactions, continuous player capsule travel, courier-route clearance, shortcut timing, controller choices, escape and replay. The initial intercept fixture stood just outside the 250 cm range; moving it 20 cm closer corrected the test without relaxing production interaction rules.

Offscreen visual review caught missing standalone lighting and clipped automatically wrapped trial text. Added dedicated daylight and explicit trial text widths, then rebuilt and reran the trial. Reviewed yard overview, pursuit HUD, full briefing, confrontation, verified outcome and escape at 1280 by 720. Screenshots are in Docs/Verification/FreightTrial. Physical-controller hardware, a packaged build and human pacing acceptance are not covered by automated tests. No player save was loaded or written.
