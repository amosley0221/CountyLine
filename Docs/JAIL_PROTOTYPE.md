# Playable jail-office prototype

## Run

Open `CountyLine.uproject` in Unreal Engine 5.8.2, open `/Game/Maps/L_JailOffice`, and press Play. The default startup map and game mode already point to this scene. Click the viewport to capture input; Shift+F1 releases the mouse in the editor.

For a separate game window using the installed editor runtime, run `Scripts/Play-Prototype.ps1`. This is an uncooked development prototype, not a packaged standalone distribution.

## Controls and walkthrough

- WASD / controller left stick: walk. Mouse / right stick: shoulder camera.
- E / controller A (bottom face button): talk to Pruitt or read the report when in range and looking toward it.
- Tab / View: County Book. Escape / Menu: pause menu. Escape / B closes a paper screen.
- In the book and conversation: mouse, arrow keys + Enter, D-pad or left stick + A. A dark outline identifies the focused control; scrolling follows focus. Disabled controls are skipped. LB/RB change book pages. B closes a screen; View/Menu also return to gameplay. Controller labels use Xbox conventions; other PC controllers need to expose compatible gamepad input.

1. Turn toward Deputy Pruitt, standing left of the route from the entrance to the desk. Press E / A and choose **HEAR HIM OUT**. The short conversation introduces the report; its wording is prototype-authored, not quoted canon. You play Acting Sheriff Sam Reed. Pruitt and Reed both use temporary mannequin art.
2. Return to the office and approach the desk. Look down toward the cream report until its prompt appears. The objective prompt follows your progress. You may read the report without talking to Pruitt first.
3. Read it with E / A, then choose **ENTER IN COUNTY BOOK**.
4. Include or omit either known fact, select one of three authored closing lines, then SIGN or HOLD. Submissions lock for this prototype and cannot be applied twice.
5. Read the Ledger page for the consequence in prose. No numeric reputation is displayed.
6. Choose **WRITE THE DATE** while at the desk. Quit and relaunch to resume the saved case. Merely signing or closing the book does not save. The completion prompt appears only when the current report and copy preference match the saved snapshot.
7. Close the book and walk around the office. The deputy, cell, and room boundaries have collision. Talk to Pruitt again for a response to the signed or held report.
8. To try the other outcome, choose **START AGAIN (UNSAVED)** in the pause menu. This starts a fresh report in memory; the previous saved date stays intact until you explicitly save again.

Saving stores the report, whether Pruitt's introduction was heard, and typed-copy preference in `Saved/SaveGames/CountyLine_JailPrototype.sav`. Existing version-1 saves remain readable; the new introduction flag defaults to false. Spawn location resets to the entrance. This is a versioned prototype slot, not the future campaign save format.

## Scope

Implemented: third-person animated placeholder, collision-aware shoulder camera, enclosed furnished graybox, range/view/occlusion-gated report interaction, report cover, County Book navigation, included/omitted facts, authored closing lines, one-time SIGN/HOLD consequences, typed/handwritten copy toggle, pause, and desk-only save.

The Map and Payroll pages explicitly identify unfinished systems. People and Ledger contain a small authored sample. Pruitt has a short text conversation; there is no voice acting, dialogue camera, branching dialogue system, full ledger subsystem, evidence investigation, world clock, vehicles, firing, combat, or wider county yet. Paper/conversation screens pause the world in this prototype; campaign time behavior remains undecided.

Reed currently uses Epic's template mannequin and locomotion. The final likeness, clothes, voice, and body rig are not implemented. The office props are geometric stand-ins. Lighting uses a simple dynamic setup for quick iteration; this is not the final Lumen/Nanite world configuration.

## Editing and rebuilding

The room is the native `ACLJailOffice` actor in an ordinary Unreal map. Its native components are visible in the editor. Materials live in `Content/Prototype/Materials`; the room layout is in `Source/CountyLine/World/CLJailOffice.cpp`.

`Scripts/build_prototype_assets.py` runs through Unreal's Python commandlet and regenerates the prototype-owned map. It replaces that generated map, so put manually authored successor levels under a new name. It preserves existing prototype materials. Build C++ before running the script.

The book is Slate, built by `SCLCountyBook`. State lives in `UCLCaseState`, not in the UI. The small state structure is a stepping stone toward the planned data-driven paper and ledger systems.
