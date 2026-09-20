# Pecos Bend: first playable town pass

Geography decision: the user has authorized the game layout to lead the final map artwork. See [WORLD_GEOGRAPHY.md](WORLD_GEOGRAPHY.md) and the [current footprint map](Maps/PecosBend-layout.svg). Lang's remains south of the square and the jail east of the courthouse; these intentionally differ from the supplied illustrated town plan.

The subsequent [block and seating revision](TOWN_LAYOUT_REVIEW.md) groups shops into street-facing rows with clear approaches and rear access. Bench views now face public space. This supersedes the original individual shop placements and side-wall seating.

## Visual target

The subsequent [architecture detail pass](TOWN_DETAIL_PASS.md) adds procedural surface finishes, façade trim, residential details and revised outdoor lighting within the approved layout.

The user's reference is `Docs/Reference/design_handoff_county_line/boards/uploads/Grok Image 2026-09-17 at 7.26.33 PM.png`, reconfirmed on 20 September 2026. It guides the courthouse silhouette, low brick storefronts, dusty street grid, utility poles, sparse shade trees, and the town's relationship to open country. Its visual quality is a production target, not a claim that this first pass reproduces its realism or full extent.

The reference can be recreated as a navigable Unreal environment through authored buildings, terrain, vegetation, materials, lighting and camera composition. A single concept image does not specify hidden building faces, interiors, accurate dimensions or exact geography. Those require additional design and asset work. The written Design Bible takes the railway around the west edge, rather than through the square; that remains the intended layout despite differences in the image. The river, railway, water tower, vehicles and distant oil field are not implemented in this pass.

## Playable now

- North Lane extends Court Street north past the courthouse to four modest homes. Explore the lane, porches and rear yards; houses and sheds have closed interiors. Brick, plaster and timber exteriors, pitched roofs, gardens and washing lines establish the first residential block.

- Exit the jail and follow the road west to Court Street and the courthouse square. The road east still leads continuously to Bend Lateral.
- The brick courthouse, cupola, five storefront exteriors, street poles, trees and signed jail frontage establish the first town silhouette. The courthouse and shops have closed interiors.
- Lang's boardinghouse is south of the square. Walk through its open doorway into the ground-floor lobby, then inspect the guest register at the counter with E or controller A. B/Escape or the return button restores movement.
- Court Street and Lang's are recorded on the Book's Map page when Reed reaches them while grounded. Both have safe checkpoints and are persisted with the existing world state when the date is written at the jail desk.

The register notice is prototype-authored text, not a new conversation with Mrs. Lang. Rooms, sleeping, meals, tenancy, a hotel save station and scheduled NPC routines remain future work. No case evidence, report decision, calendar time, money or reputation changes when reading the notice. The original investigation remains available.

## Construction and scale

`ACLPecosBend` builds the compact town from native mesh components and the project's existing materials. No map regeneration or new external asset dependency is required. Its approximately 88 by 115-metre footprint is a layout study, not the final eight-street town or literal county scale. `ACLCountyRoad` remains the initial location/checkpoint registry; town streets open the old western route boundary while outer perimeter barriers retain the slice's limits.

The next art stages are proportion/layout approval, bespoke modular brick and timber building assets, modeled doors/windows/roofs, richer terrain and vegetation, and lighting/weather refinement. The goal is the reference's grounded period look; the current simplified geometry is deliberately an intermediate playable stage.

## Verification scope

Runtime checks cover collision-swept travel from the jail to the square, entry into Lang's and return; location discovery; a safe lobby checkpoint; normal range/view/occlusion gating for the register; controller opening/closing; preservation of the original carbon; and memory-only serialization of the new location ids and checkpoint. Existing report, evidence, follow-up, save-validation and route tests remain required. Tests do not use the player's save slot.

## Rendered previews

Verified Unreal captures: [town overview](Verification/PecosBend/TownOverview.png), [residential lane](Verification/PecosBend/ResidentialLane.png), [jail frontage](Verification/PecosBend/JailFrontage.png), and [Lang's lobby](Verification/PecosBend/LangLobby.png).

For a fresh capture, create `Saved/Screenshots/TownReview`, then launch the editor executable with the project and `/Game/Maps/L_JailOffice -game -windowed -ResX=1440 -ResY=900 -nosplash -CLSmokeTest -CLTownReview`. After the smoke checks pass, the game takes ten fixed-camera screenshots and eight resident/Book/Pruitt screenshots, then exits. This development-only mode does not use the player's save slot.

The initial town pass passed 10 automation suites and 68 runtime smoke assertions. Subsequent jogging and residential work expands that coverage; see [VERIFICATION.md](VERIFICATION.md) for current results and limitations.

The first North Lane resident conversation is documented in [RESIDENT_ENCOUNTER.md](RESIDENT_ENCOUNTER.md).
