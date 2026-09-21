# Courthouse clerk lobby

The courthouse's south entrance now opens into a ground-floor public lobby. Its footprint, street-facing facade, upper storey, cupola and exterior benches retain their positions. The former solid lower block was replaced with walls around a real doorway, a level floor, interior plaster, lobby seating, a lit counter, paper and a stamp. Upper rooms remain inaccessible.

Inez Padilla stands behind the counter as an original static character proxy, approximately 162 cm tall. Her county-clerk role and stature follow the supplied design bible; the dialogue and lobby arrangement are prototype-authored. The figure is not final likeness, rigging, animation or voice work.

## Playing the visit

Leave the jail, follow the road west to the square, then enter the courthouse's south doorway. Approach the counter, look toward Inez and press E / controller A.

- Draft: Inez asks Reed to finish the report at the jail desk.
- Signed or held: choose **Present the signed report** or **Present the held report**. She reviews the disposition, included/omitted counts and whether the resident account belongs to the original carbon or was learned later.
- B / Escape leaves; movement returns. The path out leads back to the square and jail desk.

This is a clerk review, not a new filing system. It creates no receipt, hearing, reputation change, autosave or replacement carbon. No new save field/version or location id is needed. The lobby remains within Court Street's existing discovery region and uses its existing safe checkpoint. The Book's Map and People pages and the development schematic now identify the open clerk lobby.

## Verification

The `CountyLine.Report.ClerkReview` automation suite covers unsigned responses, signed/held dispositions, included/omitted/later resident notes, repeated reviews and whole-report immutability. Runtime smoke sweeps Reed's capsule through the actual entrance to the counter and back to the jail desk, checks normal interaction gating, presents each report status using controller input, and verifies input restoration and unchanged report data. The fixture never reads or writes the player's save slot.

`-CLSmokeTest -CLTownReview` includes clerk-lobby, draft, signed and held review captures. Visual captures are development fixtures; no player save is used.
