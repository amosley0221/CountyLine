# Design baseline

## Confirmed directly by the user

- Title: The County Line.
- Platform: PC; perspective: third person; period: 1926–1928.
- Open-world direction confirmed in conversation; current playable areas remain compact development slices.
- The user authorized choosing coherent game geography and refining the final illustrated maps to match it. `WORLD_GEOGRAPHY.md` records the adopted layout and future district reservations; conflicting reference-map placements are not binding.
- Pecos Bend's visual target is the supplied aerial town image, reconfirmed on 20 September 2026. See `PECOS_BEND_TOWN.md` for its relationship to the current town pass and written guide.
- Engine: Unreal Engine 5.8 (local installation is 5.8.2).
- Repository: https://github.com/amosley0221/CountyLine.
- Codex handles most of the project; Claude Code assists with some work.

## Working design from the attached handoff

Update: the subsequently supplied Design Bible revises several of these details. See `DESIGN_REVISION_NOTES.md` before implementing campaign, vehicle, map, or succession systems. The jail scene is a standalone prototype, not a final decision about the campaign opening.

The following is reference-derived, not an additional instruction from the user. Preserve it as the working baseline until the user changes it. The reference's “canon lock” describes its author's design intent; it does not override user direction.

- Single-player narrative lawman simulation; Sam Reed, acting sheriff of Rivas County, West Texas, October 1926–November 1928.
- Badge and Fired paths share the county. Firing removes county property and authority, not Reed's compass or continued play.
- Four internal ledgers: Courthouse, Street, Capital, Home. Feedback uses authored prose; never show numeric reputation meters.
- Reports retain included and omitted known facts and one authored closing line. No free-text report generation.
- County Book pages: Ledger, Map, Cases, People, Payroll. Paper presentation, minimal HUD, no minimap, no player dot on the map.
- First hour: jail report → Court Street → Lang's → Bend Lateral evidence → hearing → newspaper consequence. No required combat.
- Accessible typed alternative to handwriting, with identical content.
- Long-term aspiration: 20–30 hours and a 38 × 26 mile county. These are design ambitions, not an estimated or funded production commitment.

## Questions to resolve before affected systems are implemented

1. Map scale: literal playable distances versus compressed geography with authored travel time. Prototype compact spaces first.
2. Radio: README/HUD mentions crackle and a “Sheriff” answer, but board 03 explicitly says no car radio in 1926 and defines “radio” as depot wire. Prototype telephone / telegraph relays; research period communications before final audio and props.
3. Derrick count: map notes request one at game start; Camp A art notes request two rigs. Keep undecided until Camp A production.
4. Fast travel: README says roads already driven; board 03 includes ridden routes. Keep the route-unlock rule configurable until settled.
5. Deputy succession: Vines may quit when Street or Courthouse collapses, but firing also makes him acting sheriff. Define precedence and a fallback before implementing firing.
6. Time: first-hour timestamps mix player duration with travel/game time. Define calendar start, clock rate, time-skips, and journal pause behavior before scheduling hearings or 17:00 firing.
7. Ledgers: threshold values, “recent” incident window, and commissioner weights are incomplete. No invented numbers should become canon silently.
8. Saves: clarify which jail access and sleep locations survive firing and whether hotel manual saves require any conditions.
9. PC launch requirements: initial development targets Windows; minimum hardware, distribution store, controller support, and accessibility scope remain to be specified.

These decisions do not prevent the initial movement, interaction, and report prototype.
