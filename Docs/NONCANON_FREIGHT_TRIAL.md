# Non-canon freight-yard playtest

This is a disposable gameplay experiment requested by the user, not a mission intended for the final game. The unnamed courier, missing shipment, clinic explanation, yard and all outcomes are invented for this test. No final-game mission, named story character, county location or case is used. Reed's existing player mesh and movement are reused; the courier temporarily uses the same mesh, labeled TEST COURIER. All geometry is a blockout.

## Play

Use the desktop shortcut **The County Line - Non-canon Playtest**, or run `Scripts/Play-Prototype.ps1 -EngineRoot <UE5.8 directory> -TestMission`. The normal launcher still starts the existing town prototype. The playtest has its own daylight and a separate yard 300 metres away; town and field actors are not spawned in this mode. `Scripts/Create-PlaytestShortcut.ps1` recreates the separate shortcut without changing the town shortcut.

The briefing opens on arrival. Close with B / Escape, or choose Return to yard. WASD / left stick moves, mouse / right stick looks, Shift / LB jogs, E / A interacts. Tab / View and Escape / Menu open the playtest screen and pause the clock. Exit Playtest closes the game.

1. Explore the start area. The dispatch desk contains an optional clue: a paid clinic shipment with an unsigned release.
2. Inspect the broken crate to start the pursuit. The courier runs around the north side of the freight shed toward the east exit. A countdown shows the remaining time.
3. Follow him around the shed or unlatch the gate and cut along its south side. Jog to gain ground. Move within interaction distance, face him and press E / A to stop him.
4. Recover the shipment, accept his claim and release it, or use the dispatch slip to require a signed handover. Verification is unavailable without the clue. Each outcome explicitly states the cost or uncertainty.
5. Read the outcome and replay. An escape also offers an immediate replay. There is no paperwork trip or save desk.

## Scope and isolation

`-CLTestMission` bypasses both loading and writing the campaign save. Trial state exists only on `ACLTestMission` in memory. The Book is replaced by trial briefing/confrontation/outcome controls; normal report pages and saving cannot be accessed. World discovery and checkpoint updates are skipped. Relaunching or replaying clears all trial progress. Existing campaign save versions and mission data are unchanged.

This tests movement under time pressure, optional investigation, a physical shortcut, interception and an evidence-dependent decision. The courier follows an authored route rather than full navigation AI. There is no combat, voice acting, theft economy, clinic simulation or confirmed delivery simulation. Outcomes are a visible debrief and cargo disposition, not a claim that those larger systems exist. This is the first gameplay pass; pacing and fun still need the user's playtest.

## Validation

`Scripts/Test-Prototype.ps1` includes the FreightRules suite alongside existing regression suites. `Scripts/Test-FreightTrial.ps1` checks save isolation, briefing/controller input, evidence/shipments/gate interaction through production range-view-occlusion checks, physical player and courier routes, timing, confrontation choices, escape and replay. Pass `-Review` for offscreen screenshots in Saved/Screenshots/FreightTrial. Tests never access the real save slot.
