# Production plan

Build a small playable loop before expanding geography. Stages below are ordered gates, not schedule promises.

## M0 — Project foundation

Create the UE 5.8 C++ module and editor/game targets, preserve source references, document design uncertainty and collaboration, and verify the editor target against the installed engine. A successful build establishes a technical baseline only.

## M1 — Jail-office graybox

Create a small playable map with third-person movement, collision-aware camera, a desk, and one interactable report. Add interaction input and a readable prompt. Use placeholder assets with recorded provenance.

Acceptance: launch in editor, walk and turn without entering first person, approach the desk, interact only within reach and line of sight, and open/close a placeholder County Book. Confirm keyboard focus returns to gameplay.

## M2 — Report and consequence loop

Implement the append-only ledger, known facts, report draft, and SIGN / HOLD. Derive newspaper copy from persisted report state. Add a save station and round-trip persistence early. Keep numerical ledger values out of player-facing UI.

Acceptance: SIGN records the reference's Courthouse +2; HOLD records −3; repeated UI activation cannot duplicate consequences. Included and omitted facts persist, and reload restores the same report and newspaper variant. Add focused automation for these state transitions.

## M3 — Investigation slice

Connect compact Court Street, Lang's, and Bend Lateral grayboxes. Implement evidence discovery, visited-place records, the hearing, and newspaper payoff. Prototype travel using transitions before spending on vehicle and horse simulation.

Acceptance: both initial report choices complete the sequence; evidence changes available authored choices; no gunfight is required; save/reload works at each supported station; leaving and returning does not reset case state.

## M4 — Presentation and travel

Implement the paper UI from the boards, typed handwriting setting, period audio, Ford prototype, surface/weather modifiers, and a short representative road. Profile on a declared PC hardware target. Validate fonts/assets and historical details before replacing placeholders.

Acceptance: a packaged Windows build completes the investigation loop, supports the chosen input devices and UI scaling, and meets an agreed performance budget.

## M5 — Loss of office

After resolving succession/time questions, implement voting, the 17:00 transition, property revocation, civilian interactions, payroll, and Lang's messages.

Acceptance: firing while in a vehicle or paper screen resolves consistently; no county vehicle access remains; compass and journal survive; save/reload preserves the path; all visible consequences agree with the minutes.

## Beyond the slice

Only expand toward the full county and multi-year campaign after a reviewed playable slice. Budget world art, animation, horse/vehicle behavior, writing, audio, QA, and optimization separately. World Partition is a later world-production decision; it is unnecessary for the jail-office prototype.

## Proposed implementation boundaries

- C++: authoritative state, report/ledger rules, persistence, interaction contract, clock and duty transitions.
- Blueprint: scene assembly, authored interactions, animation and presentation hooks.
- UMG: County Book, prompts, paper screens, accessibility presentation.
- Data assets/tables: names, facts, closing lines, thresholds, remarks, and newspaper templates.

Build only the dependencies needed by the current stage. Reference class names are proposals, not a requirement to generate empty systems up front.
