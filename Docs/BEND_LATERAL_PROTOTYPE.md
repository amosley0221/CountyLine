# Bend Lateral playable study

This is prototype-authored dialogue and graybox staging, not a replacement for the design bible or a final county layout. Sam Reed remains the player. Salazar uses temporary mannequin art; the bottle, bank, road and channel use simple geometry.

## Play

Launch the existing desktop shortcut. Approach the office entrance door (opposite the desk), look at it and press E / controller A. Choose TAKE THE ROAD. Speak to Salazar and inspect the BOTTLE and DITCH BANK markers. Choose ENTER FIELD NOTE at each. The JAIL OFFICE sign returns Reed to the office. Review CASES in the County Book, include or omit the available field notes, choose a closing line, then SIGN or HOLD. WRITE THE DATE at the desk saves report and field notes.

A submitted report stays locked. Further investigation adds notes but never rewrites its carbon. To explore an alternative submission, START AGAIN resets the in-memory case; it does not replace the saved slot until the date is written again.

The outdoor area is spawned 100 metres away in the prototype map. Door/sign interactions move Reed between the two bounded areas. This is a temporary transition, not the final travel system. New reflected report fields default safely for older Version 1 saves. The selected closing wording is frozen when submitted.

## Shared work

Claude owns the separate save-validation helper, new tests and runner update. See CLAUDE_SAVE_VALIDATION_PROMPT.md. Codex owns case-state integration, gameplay, UI and field art in this branch.

## Verification — 2026-09-19

UE 5.8.2 CountyLineEditor Win64 Development built with -NoPCH -NoUBA. All three CountyLine automation suites passed, including field-evidence serialization, inclusion/omission and frozen carbon wording. All 29 runtime smoke assertions passed, including simulated controller travel, cancel without recording, three unique notes, immutable submitted carbon, and returning with notes intact. No physical controller was available for hardware testing.

Visual playtest confirmed the office-door prompt and outbound transition, corrected outdoor exposure, and the world-space Salazar interaction and note panel. Final character/environment art, vehicles, seamless travel, expanded dialogue and report amendments remain future work.
