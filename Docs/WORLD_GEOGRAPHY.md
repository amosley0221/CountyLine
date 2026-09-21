# Rivas County geography

## Decision and authority

On 20 September 2026 the user authorized choosing a coherent game layout and subsequently refining the illustrated maps to match it. The playable world's approved layout will lead future map artwork. The two supplied maps are visual/design references, not exact surveying data or additional instructions.

Keep the current town arrangement: courthouse northwest of the jail, Lang's south of the courthouse square, and the investigation road leaving town east toward Bend Lateral. This supports the opening jail-to-square-to-Lang's-to-investigation loop without relocating the working office, interactions or save checkpoints. The illustrated town and county maps disagree about the jail's position; neither overrides this arrangement.

For map authoring, Unreal +X is east, +Y is north and +Z is up. Coordinates below are centimetres. This convention describes the world plan; it does not add an in-game compass.

## Current playable town

See [the current footprint map](Maps/PecosBend-layout.svg). Solid footprints represent existing buildings, not promises of accessible interiors. Numbers correspond to its key. The drawing is a development schematic, not the final County Book artwork.

| Landmark | Current centre X, Y | Role |
| --- | --- | --- |
| Courthouse | -3900, 1200 | Main visual anchor; ground-floor clerk lobby open |
| Jail office | 0, 0 | Report, deputy and manual save |
| Lang's | -4200, -2200 | Accessible lobby and register |
| Court Street | -2200, -100 | North/south walking spine |
| Square crossroad | -3650, -800 | Connects square, jail approach and east road |
| Bend Lateral | Approximately 10000, 0 | Existing investigation area east of town |
| North Lane | -2800, 4300 | Residential lane linked to Court Street |
| Residential homes | X -5500, -3650, -1250, 400; Y 5550–5650 | Four exterior-only homes with porches and yards |

The first residential block extends the northern town boundary to Y 7400. It is treated as part of Court Street for discovery and uses the existing Court Street recovery point; no save ids or existing checkpoints moved. Rear yards have sheds, vegetable plots and washing lines. An unnamed River Road resident can be heard at the west house; accessible house interiors remain future work.

The five existing shops retain their prototype identities for now. Their eventual business names and architecture can change as the town expands. Do not silently treat the existing repair shop as an implemented hotel or livery.

Following the user's street-level review, the commercial plan now takes priority over the illustrations: repairs, general store and dry goods form an east-facing row on Market Street, with a rear service lane. Drugs and post office form a west-facing row on Court Street. Four courthouse benches face the forecourt or north green with their backs toward the building. See [TOWN_LAYOUT_REVIEW.md](TOWN_LAYOUT_REVIEW.md). The west boundary is now X=-7640; the civic buildings and residential plots remain in place.

## Reserved county geography — future construction

These are design allocations, not existing playable destinations or exact coordinates:

- West town edge: depot and railway, outside the courthouse square. Tanner's spur branches west/northwest toward the lease and oil camps.
- South of town: River Road and the Rivas River corridor. The main river remains distinct from the small Bend Lateral irrigation channel.
- Southeast: Cottonwood Crossing, with a road connecting the town and river corridor. Its ford requires a later traversal design.
- East/northeast: open range and Voss's fenced land. The existing east investigation road can become the first branch into this district.
- Town expansion: reserve southern blocks for Bend Hotel, The Enterprise and the livery; a quieter eastern block for the chapel; commercial frontage around the square for the bank, mercantile and doctor. Their precise plots must be placed and walked before illustration.

Keep the courthouse visible on town approaches and preserve a direct pedestrian route between jail, square and Lang's. Final county distances remain undecided: the current town and roughly 100-metre investigation connection are compressed prototypes, not a literal representation of the reference maps' printed scales.

## Keeping artwork consistent

1. Place and playtest each district in Unreal, including doors, roads, collision and sightlines.
2. Update the footprint drawing and this location register from the implemented geometry. Run `Scripts/draw_town_layout.py` to rebuild the current schematic from the town source.
3. Once a district's layout is stable, refine its parchment illustration using the approved footprints and connections. Preserve the originals as reference; do not label unbuilt districts as playable.
4. Check compass, relative placement, names, road junctions and bridges against the game. Illustration may simplify distances for legibility but must preserve connections. Do not print a literal scale until world scale is settled.

The final illustrated town and county maps are deferred until their geography is stable. This decision adds no travel, buildings, NPCs or gameplay systems by itself.
