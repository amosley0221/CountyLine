# Playable jail-office prototype

## Run

Open `CountyLine.uproject` in Unreal Engine 5.8.2, open `/Game/Maps/L_JailOffice`, and press Play. The default startup map and game mode already point to this scene. Click the viewport to capture input; Shift+F1 releases the mouse in the editor.

For a separate game window using the installed editor runtime, run `Scripts/Play-Prototype.ps1`. This is an uncooked development prototype, not a packaged standalone distribution.

## Controls and walkthrough

- WASD / controller left stick: walk. Mouse / right stick: shoulder camera.
- E / controller A: read the report when in range and looking toward it.
- Tab / View: County Book. Escape / Menu: pause menu. Escape / B closes a paper screen.
- In the book: mouse, arrow keys + Enter, or controller navigation; shoulder buttons change pages. Tab and Escape close the book even when a button has focus.

1. Walk forward from the entrance toward the desk. Look down toward the cream report until its prompt appears.
2. Read it with E, then choose **ENTER IN COUNTY BOOK**.
3. Include or omit either known fact, select one of three authored closing lines, then SIGN or HOLD. Submissions lock for this prototype and cannot be applied twice.
4. Read the Ledger page for the consequence in prose. No numeric reputation is displayed.
5. Choose **WRITE THE DATE** while at the desk. Quit and relaunch to resume the saved case. Merely signing or closing the book does not save.
6. Close the book and walk around the office. The cell and room boundaries have collision.
7. To try the other outcome, choose **START AGAIN (UNSAVED)** in the pause menu. This starts a fresh report in memory; the previous saved date stays intact until you explicitly save again.

Saving stores the report and typed-copy preference in `Saved/SaveGames/CountyLine_JailPrototype.sav`. Spawn location resets to the entrance. This is a versioned prototype slot, not the future campaign save format.

## Scope

Implemented: third-person animated placeholder, collision-aware shoulder camera, enclosed furnished graybox, range/view/occlusion-gated report interaction, report cover, County Book navigation, included/omitted facts, authored closing lines, one-time SIGN/HOLD consequences, typed/handwritten copy toggle, pause, and desk-only save.

The Map and Payroll pages explicitly identify unfinished systems. People and Ledger contain a small authored sample. There is no full ledger subsystem, evidence investigation, world clock, vehicles, dialogue, firing, combat, or wider county yet. Journal/pause screens pause the world in this prototype; campaign time behavior remains undecided.

Reed currently uses Epic's template mannequin and locomotion. The final likeness, clothes, voice, and body rig are not implemented. The office props are geometric stand-ins. Lighting uses a simple dynamic setup for quick iteration; this is not the final Lumen/Nanite world configuration.

## Editing and rebuilding

The room is the native `ACLJailOffice` actor in an ordinary Unreal map. Its native components are visible in the editor. Materials live in `Content/Prototype/Materials`; the room layout is in `Source/CountyLine/World/CLJailOffice.cpp`.

`Scripts/build_prototype_assets.py` runs through Unreal's Python commandlet and regenerates the prototype-owned map. It replaces that generated map, so put manually authored successor levels under a new name. It preserves existing prototype materials. Build C++ before running the script.

The book is Slate, built by `SCLCountyBook`. State lives in `UCLCaseState`, not in the UI. The small state structure is a stepping stone toward the planned data-driven paper and ledger systems.
