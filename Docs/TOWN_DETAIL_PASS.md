# Town architecture detail pass

This pass refines the approved town layout without moving building footprints, doors, location zones or checkpoints. The reference images remain the long-term visual target; this is an intermediate architectural pass, not finished photoreal scenery.

## Visible changes

- Centimetre-scaled running-bond brick and mortar, pale timber siding, timber grain and board joints, stone courses, metal roof seams and opaque blue-grey window glass.
- Courthouse floor band, alternating corner stones, window sills, lintels and sash bars; six benches along the side walks.
- Storefront corner trim, window sills, transoms and door handles.
- Residential gable boards, side windows, corner trim, roof ridge caps, door panels and handles, and porch rails with open central approaches.
- The shared outdoor sun now lights the south-facing façades, with stronger sky fill to make shaded detail readable. This affects Bend Lateral as well as the town; it remains fixed prototype daylight, not a time-of-day system.

Existing house and shop interiors remain closed. The decorations add no resident schedules, conversations or save stations. Foliage, terrain and many building forms remain simplified. Materials supply colour and roughness, not texture-derived normal maps or displacement; windows are opaque surfaces rather than simulated interiors.

## Reproduction

`Scripts/build_town_materials.py` creates six original Unreal materials in `/Game/Art/Town/Materials`. Run it through Unreal's Python commandlet. It edits only these materials, not maps or shared prototype materials. The source contains the procedural shader patterns and uses no downloaded textures. Surface patterns use world coordinates at fixed centimetre scales, so they remain consistent across differently scaled building components. Moving a component changes pattern phase.

`ACLPecosBend::Shape` assigns the finishes within the town actor and retains the original material when a generated finish is unavailable. Lobby inner plaster keeps its previous finish. Architectural details are native components; no map regeneration is needed. The checked-in material assets are tracked with Git LFS.

The existing `-CLSmokeTest -CLTownReview` mode now records six views, adding `CourthouseDetail.png` and `HomeDetail.png`. Use `-Multiprocess -RenderOffscreen -unattended` for offscreen review. See `VERIFICATION.md` for build, route and render results.
