# Handoff: The County Line — vertical slice (UE5, third-person)

## Overview
Narrative open-world lawman sim. Rivas County, West Texas, Oct 1926 – Nov 1928. Player is Acting Sheriff **Sam Reed** (he/him, mid-40s). Two paths, one county: keep the star (Badge) or lose it (Fired) — the map stays loaded either way. This package covers the **vertical slice**: HUD, journal ("County Book"), six paper screens, map spec, six location sheets, verbs, first hour, and the Unreal module list.

## About the design files
`boards/*.dc.html` are **design references built in HTML** — they show intended look, copy and behavior. They are not runtime code. Recreate them in Unreal (UMG / Slate, C++ + Blueprint) using the module names below. Unity alternates are noted per module on board 03; if Unity is chosen, map UMG→UI Toolkit, subsystems→ScriptableObject services, ChaosVehicle→WheelCollider.

Open `boards/01-onesheet-system.dc.html` first; boards link to each other via the top nav. Board 02 has live controls (journal tabs, Enterprise "signed/held", `clerkTypedCopy` tweak).

## Fidelity
**High-fidelity for UI** (colors, type, sizes, copy, states are final for the slice). **Reference-level for world art** — the six frames in `uploads/` are AI concept stand-ins, canon for palette/light/composition only; each location sheet lists what must change (sign text, derrick count, telephone model, etc.).

## Canon lock (do not reopen)
- Sam Reed, he/him. UI address: "Reed" in paper/journal, "Sheriff" while OnDuty, "Mr. Reed" after firing. Subtitle tag `REED`. No character creator; outfit states duty / weather / fired.
- Predecessor Hal Voss (resigned "for his health"). **No McCready.** Printed sheriff line: `RIVAS COUNTY / ACTING SHERIFF S. REED`.
- Cast: Judge Helm · County Clerk Inez Padilla · Deputy Earl Vines (only deputy; quits if Street or Courthouse collapse) · Mrs. Lang (boarding house) · Doc Ellison · Camp physician (unnamed). Invented-and-strikable names are marked "(mine)" in the board 03 gazetteer.
- Compass is Reed's own; survives firing. Firing revokes: county Ford, jail keys, star, "Sheriff" radio answer, Court St reserved parking.
- Reports: known facts included/omitted (omissions stored) + one of three authored closing lines. **No free text.**
- Ledgers (Courthouse / Street / Capital / Home): never numeric in UI; the HUD never shows the four words. Thresholds surface as clerk remarks + faint pencil tick.
- Save: sleep (jail cot, Lang's, rented room when fired) or manual "Write the date" at those + hotel night clerk. Pause = Options + Journal.
- Combat: revolver (hip), Winchester (Ford), shotgun (jail rack). No dead-eye, no weapon wheel. Any gunfight writes a Courthouse entry.
- Sound: no score on the square. Room tone, Ford, rail, wind, bell, telephone, typewriter. One later diegetic radio source.
- Prohibition quiet; hotel basement is **doorway only** in the slice (interior = Act II).
- Scope: single player, third-person only, 20–30 h. No wanted level. Reed can die; reload copy "The county wrote a different report."
- Promotion flag: `Courthouse ≥ T && Capital ≥ T && VoteScenePassed`. Firing flag: `CourthouseCritical || (CapitalCritical && PublicIncidentRecent)`. Fire resolves at **17:00 game time** that day: Ford unpossessable, star removed, Payroll stamped UNPAID, Vines acting. Helm never breaks a tie. Weights: Whitlock = Capital + Payroll; Salazar = Street.

## Design tokens (name → sRGB / UE linear)
Use these names for UMG styles AND material instances.
- Oil black `#15120e` (0.007,0.006,0.003) — HUD text ground, night
- Ink `#2a2520` (0.023,0.018,0.014) — printed type on paper, outlines
- Carbon `#3e3850` (0.050,0.041,0.082) — typewritten body (purple-grey)
- Ledger cream `#efe5cc` (0.863,0.776,0.610) — primary paper; primary text on dark
- Dust `#c8b48c` (0.578,0.456,0.262) — secondary text, rules, page backs
- Newsprint `#f3efe4` — The Enterprise only
- Brick `#8a3b2e` — courthouse, jail, fired-path rule
- Stamp red `#9e2f24` — CLOSED / DENIED / UNPAID / TERMINATED; overheat tick. Never "danger"
- Filament `#e3a449` — the only glow: focused prompt, live line, lit bulb
- Cottonwood `#5c7752` — map river margins only
- Brass `#a98b4c` — fasteners, star; gone on Fired path

Type (1080p): **Old Standard TT** headlines (display 64 / h1 44 / h2 30 / deck italic 22). **Courier Prime** body (body 20 / label 16 / fine 14 min) in Carbon. **Caveat** 28–32 for Reed's hand only. Required setting `bClerkTypedCopy`: every Caveat style → Courier Prime/Carbon, same strings.

Chrome: paper has square corners; off-desk one hard 2px offset shadow in Dust. Stamps rotate −6°…+4°, 85% opacity. Buttons: outline = available, filled Ink = focused, dashed + "(reason)" = blocked. Focus = pencil underline L→R 90 ms. Prompt key box 32px, verb 22px, object 18px, anchored 24px right of object screen point, 2.5 m / 30° gate, 120 ms fade. HUD minimum text 22px.

## Screens
1. **HUD — On duty, on foot** (board 02/B1): compass tick top-center (18px ticks, N in Filament 22px, off in interiors, user-toggle); interaction prompt; incoming-line slip bottom-right (Filament label pulsing 1.6 s, 20px body, never modal, expires on leaving block). No star widget, no minimap.
2. **HUD — In the county Ford** (02/B2): route note (Reed's pencil on cream slip, bottom-left, written when a destination is chosen); road state line bottom-right "surface, condition" (no speed); 40px temperature tick under it → Stamp red on overheat. Only HUD gauge.
3. **HUD — Fired** (02/B3): same street; county verbs struck at 60% with civilian substitute ("Drive" → coupe at Holt's; "Serve paper" → "Ask"); incoming line sender becomes MRS. LANG; compass remains; radio crackles, never answers "Sheriff". Grade −30% saturation, no warm key.
4. **County Book** (02/C): Tab opens; world dims −1.5 EV, no blur. 1020×560 @1152 board scale (≈1700×933 @1080p) cream book, brass pin, five index tabs: **Ledger** (4 columns of dated Caveat entries with +/− in margin, "carried" total, one authored sentence per column, NOTES + "Heard in the hall" threshold remarks), **Map** (1927 wall map; pencil ticks at visited places only; Reed's notes anchored where he stood; no player dot), **Cases** (report list with stamps; composer = known-fact checklist + 3 closing lines + SIGN / HOLD / AMEND(dashed)), **People** (name, ledger tag, role, Reed's note), **Payroll** (line items vs appropriation; DEPUTIZE dashed when no budget; UNPAID stamp on Fired).
5. **Pause strip** (02/D1): dust paper strip: BACK TO IT · COUNTY BOOK · OPTIONS · WRITE THE DATE (struck when not at a save station, last-written date beside) · QUIT TO DESK; date/time/place in Caveat right.
6. **Hearing stub** (02/D2): docket header, three choices (outline / filled / dashed) with Reed's pencil guess of the ledger consequence (may be wrong), Helm's line, RECEIVED·CLERK stamp.
7. **Jail register** (02/D3): 7-column ledger page; BOOK writes a line from known facts; RELEASE signs the column; meals 35¢.
8. **The Enterprise** (02/D4): 5-column masthead; lead story templated from the Cases stamp (two authored variants: signed / held); sidebars on world clock; collectible.
9. **Commissioners minutes** (02/D5): motion, four per-commissioner votes each with a ledger cause in pencil, result, property-return clause, SERVICES TERMINATED stamp. Retention variant: "fails 2–2, Judge Helm declining to break the tie."
10. **Boarding-house phone list** (02/D6): Fired-path job board; crossed-off numbers = who won't take your call; MESSAGES block in Filament label.

## Map spec (board 03)
38 × 26 mi. Pecos Bend = **eight-street grid ~1 mi²** around the brick courthouse; rail N–S through the grid, depot on its east edge, water tower by the tracks, siding west toward the camps. West: Tanner Lease, Camp A Main, Camp B, camp physician, pipeline grade (proposed), **Mile 18** on the caliche camp road. East: Open Range — empty grass, windmill line, half-built 1925 fence, Voss place, schoolhouse. South belt: Rivas River; **Cottonwood Crossing** (legal ford, River Rd, painted county-line post near bank, farms + Sacred Heart chapel far bank); **state line SW along the river west of Cottonwood** — painted post + dotted line in water, unnamed NM county, no hard border graphic; **Bend Lateral** (Act I body) ESE of the square between town and Open Range. Low Water is never drawn. Title block "RIVAS COUNTY — TEXAS / Pecos Bend County Seat / County Clerk's Office, 1927". No icon legend. Sign copy fixes: Cottonwood post reads `COTTONWOOD CROSSING / RIVAS COUNTY LINE`; office sign `RIVAS COUNTY / ACTING SHERIFF S. REED` (Voss ghosting).

Travel: Court→Camps 40–50 min dry, 2–4 h after rain. Ford: caliche 1.0, gumbo 0.15 + stall; horse: caliche 0.5, gumbo 0.7, fence 0.9. Fast travel only by depot ticket or roads already driven; arrival is a cut with clock advanced.

## Proposed folder tree (UE5)
```
CountyLine/
  Source/CountyLine/
    Camera/     CLFollowCameraComponent.{h,cpp}
    Interact/   CLInteractionTraceComponent.{h,cpp}  ICLInteractable.h
    Ledger/     CLLedgerSubsystem.{h,cpp}  CLLedgerTypes.h  (FLedgerEntry, ELedger, thresholds DataTable row)
    Duty/       CLDutyStateComponent.{h,cpp}  (EDutyState, county property set, promotion/firing flags)
    Paper/      CLPaperSubsystem.{h,cpp}  CLReportComposer.{h,cpp}  CLPaperTypes.h (FPaperDoc, FKnownFact, FReportDraft)
    Travel/     CLTravelModifierSubsystem.{h,cpp}  (surface enum, weather curve)
    Combat/     CLCombatComponent.{h,cpp}
    Save/       CLSaveStationComponent.{h,cpp}  CLSaveGame.h
    Gazetteer/  CLGazetteerDataAsset.{h,cpp}
  Content/
    UI/CountyBook/   WBP_CountyBook  WBP_Page_Ledger/Map/Cases/People/Payroll  WBP_PauseStrip
    UI/HUD/          WBP_HUD  WBP_Prompt  WBP_CompassTick  WBP_IncomingLine  WBP_RouteNote  WBP_TempTick
    UI/Paper/        WBP_HearingStub  WBP_JailRegister  WBP_Enterprise  WBP_Minutes  WBP_PhoneList
    UI/Fonts/        OldStandardTT  CourierPrime  Caveat
    UI/Styles/       DA_CLColors (token names above)  DA_CLType
    Vehicles/        BP_CountyFord  BP_CivilianCoupe  BP_Horse
    Actors/          BP_CommissionersVote  BP_SaveStation_*  BP_PaperPickup  BP_DepotWire
    Data/            DT_LedgerThresholds  DT_ThresholdRemarks  DT_KnownFacts  DT_ClosingLines  DT_EnterpriseTemplates  DA_Gazetteer
    Maps/            L_RivasCounty (World Partition)  L_PecosBend_Interiors
```

## Unreal modules
- **UCLFollowCameraComponent** (C++). SpringArm 320 cm, socket offset (55,0,65), lag 8/12. Presets OnFoot / InFord (rear-seat, longer arm) / OnHorse / Interior (short, collision-clamped). Blend 0.4 s. No free look past 75°. No first-person.
- **UCLInteractionTraceComponent** (C++). Sphere sweep from camera, 2.5 m, 30° cone, 10 Hz. `ICLInteractable::GetPrompt(DutyState) → {Verb, ObjectName, bStruck, StrikeReason}`. Owns WBP_Prompt; anchors 24 px right of object screen point.
- **UCLLedgerSubsystem** (GameInstance subsystem). Four append-only logs `FLedgerEntry{Date, CauseKey, Delta, Source}`; totals computed, never stored or shown. `DT_LedgerThresholds` rows: Promotion, Firing, Critical, RebelViability. Events `OnLedgerEntry`, `OnThresholdCrossed(ELedger, RemarkKey)`. Per-commissioner weight map lives here.
- **UCLDutyStateComponent** (PlayerState, C++). `EDutyState {OnDuty, Fired}`, county property set, address form, promotion/firing flags per canon. Only `BP_CommissionersVote` mutates it, at 17:00 game time.
- **WBP_CountyBook** (UMG). One widget, five pages, pause strip. Map page = wall-map texture + pencil-tick decals from `VisitedPlaces`. Honors `bClerkTypedCopy`. World dim via post-process weight, never blur.
- **UCLTravelModifierSubsystem** (World subsystem). Surface × weather → speed multiplier + stall chance per pawn type; rain raises gumbo depth over 6 h, dries over 18 h; feeds HUD road-state line and fast-travel duration.
- **BP_CountyFord** (ChaosVehicle pawn). Possess gated on `OnDuty`; on Fired `bCanBePossessed=false`, remains in world as Vines's. Holds Winchester. Dash gauges are meshes; HUD temp tick only.
- **UCLPaperSubsystem + UCLReportComposer** (C++). `FPaperDoc{Type, Fields, Stamp, CarbonOwner, IncludedFacts, OmittedFacts, ClosingLineIdx}`. Enterprise templater picks lead from the Cases stamp; minutes compose from ledger causes. Every doc is a pickup actor with a rendered page material.
- **UCLIncomingLineComponent** (HUD, BP). Queue `{Sender, Line, Expiry, AnswerLocation}`; sender DEPOT → MRS. LANG on Fired; radio never answers "Sheriff" once fired.
- **UCLCombatComponent** (C++). Three fixed weapons; visible only when drawn; any discharge in a named place → `LedgerSubsystem.Add(Courthouse, GunfightAt_<Place>, −)`.
- **UCLSaveStationComponent** (BP). Tags JailCot / LangHouse / HotelNightClerk / RentedRoom(Fired). Sleep or "Write the date". Death → reload with the canon line.
- **BP_CommissionersVote** (actor). Reads ledgers at session dates, resolves per commissioner via weights, writes minutes doc, applies DutyState change and world side-effects (sign repaint, Ford, Payroll UNPAID, Vines acting).
- **UCLGazetteerDataAsset**. Single source of names for map labels, prompts, journal, Enterprise.

## First-hour mission graph
```
[0:00] JailOffice_Morning  — spawn seated; star on desk; report 26-001 on top (CLOSED, unsigned)
   │   teaches: Tab (Book), Wait, Sit           ledger: —
   ▼
[0:04] Report_SignOrHold  — Cases composer: facts known so far {Salazar found him, bottle} ; SIGN | HOLD | AMEND(dashed)
   ├─ SIGN → Courthouse +2 ; Enterprise variant = signed ; branch rejoins at [0:08]
   └─ HOLD → Courthouse −3 ; Enterprise variant = held
   ▼
[0:08] CourtStreet_Walk  — clerk's window (Padilla); depot wire: "Helm before noon" (incoming line)
   │   teaches: prompt gating, incoming line   ledger: —
   ▼
[0:15] LangHouse  — Knock → Sit: board terms, phone list, Voss's coat on hook; People entries Lang, Salazar
   │   teaches: Knock, Sit, People tab          ledger: Home +1
   ▼
[0:24] Travel_ToLateral  — choose Ford (motor shed) or horse (livery); route note; dry: 12 min; Ford stalls once in sand (no fail)
   │   teaches: Drive/Ride, route note, travel modifier
   ▼
[0:36] BendLateral  — Salazar present or arrives on Wait; Search: bottle full/sealed, boot prints above waterline, no wet clothes
   │   adds KnownFacts {BottleSealed, PrintsAboveWater, NoWetClothes}; Map: first pencil tick + note
   │   optional: Doc Ellison call at Lang's → {NoWaterInLungs, MarksOnThroat}
   ▼
[0:48] Travel_Back  — first fast travel offered (road now driven); clock jump; late → second wire
   ▼
[0:55] Helm_Hearing  — WBP_HearingStub: AFFIRM | HOLD | NAME_TYPIST(dashed unless fact known)
   │   Helm: "the county buries a man for eleven dollars…"
   ▼
[1:00] Enterprise_Thursday  — front page renders from Cases stamp; end of first hour. No gunfight anywhere in this graph.
```

## Assets
`uploads/` — six AI concept frames (courthouse noon, jail night, Camp A, hotel kitchen, aerial, Cottonwood bank-level) + reference wall map. Concept only; not shippable. Fonts: Google Fonts Old Standard TT, Courier Prime, Caveat (OFL).

## Files
- `boards/01-onesheet-system.dc.html` — one-sheet, tokens, type, chrome, do/don't
- `boards/02-hud-journal-screens.dc.html` — three HUD states, County Book (interactive), six paper screens
- `boards/03-map-locations-production.dc.html` — map redraw list, districts, six location sheets, verbs, first hour, UE module list, locked decisions, gazetteer
- `boards/support.js` — runtime the boards need to open locally
