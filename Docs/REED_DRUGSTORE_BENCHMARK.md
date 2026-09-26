# Reed and drugstore visual benchmark candidate

This pass establishes a more complete authoring pipeline for one character and one storefront. User approval of the resulting appearance is still required before treating it as the standard for the rest of the cast/county. It is not a claim of final photorealistic likeness, hand-retopologized production topology or a finished open world.

## Reed

`Scripts/author_reed_benchmark.py` reads the earlier editable Reed scene and builds a separate candidate. Coat pieces are joined through a remeshed surface; skin, neck, ears and hand/finger junctions are fused where they meet. The face has an integrated nose/brow/cheek profile instead of a separate nose sphere. Accessories remain separate surfaces where that is appropriate. Weights are transferred from the existing rig, blended at sleeve roots, limited to four influences and normalized. The final base mesh has a 45,000-triangle budget.

An unwrapped 2048-pixel base-color atlas and packed roughness/metallic atlas replace the separate material sections. Both are baked from original material definitions in Blender; no downloaded textures or meshes are introduced. Unreal generates three distance LODs; the drugstore meshes also have three distance levels. Android retains the base level because the forced reduced-LOD close-up showed hat-band artifacts; stripping it is deferred until measured device review. The old Reed asset and source remain available for comparison, while the player uses `SK_Reed_Benchmark`.

This is still a procedural interpretation of IMG_1940.JPG. The source portrait has greater facial, cloth and skin fidelity than this candidate. Final likeness sculpting, intentional facial-animation topology, wrinkle/normal detail, groom/hair work, and production deformation review remain unresolved. Automated surface fusion is not a substitute for those tasks.

## Drugstore

The accepted building footprint and routes stay in place. Two new authored meshes add bevelled door panels and casing, divided upper-door glazing, window trim, stone sill/lintel work, cornice/drip edges, a single downpipe, and shaped apothecary display bottles with ruled labels. They use the existing shop frame, windows, shelves and awning. The old simple stock is hidden; collision is unchanged. This does not introduce a walk-in shop interior or medical product claims.

## Reproduction and evaluation

1. Run Blender with `Scripts/author_reed_benchmark.py` and `Scripts/author_drugstore_benchmark.py`.
2. Import through `Scripts/import_reed_drugstore_benchmark.py`.
3. Build the PC editor and run `Scripts/Test-Prototype.ps1`.
4. Run the rendered smoke review with `-CLSmokeTest -CLTownReview -CLStreetReview`. Added walk, jog and reduced-LOD views supplement the idle portraits.
5. Package Android with `Scripts/Build-Android.ps1`.

Inspect shoulders, coat hem, hands, facial texture seams, door/display layering and LOD transitions. Phone acceptance requires the actual device: walk and turn outside the drugstore, visit the jail, then play for 20 minutes and check heat/resume behavior. No frame-rate or thermal result is inferred from editor previews or successful packaging.

Claude's separate measurement assignment is in `Docs/CLAUDE_PERFORMANCE_BENCHMARK_PROMPT.md`. It owns new performance files and a narrowly scoped save guard; Codex owns the art, assets, character integration, town geometry and review cameras. No final campaign missions or cloud save changes are part of this pass.

## Validation notes

The first rendered review caught a bind-pose scale mismatch that the gameplay tests could not detect. Removing the imported rig's parent helper had removed its inherited 0.01 scale. The authoring script now preserves that world transform and asserts matching human-scale mesh/rig bounds before export. The current asset was re-exported with the corrected bind transform.

The first full test log ended before the last two results were persisted. The isolated storefront test passed; enabling `-forcelogflush` in the test runner produced a complete rerun with all 20 Unreal suites, 10 map tests and 134 runtime checks passing. This changes test logging, not assertions or gameplay. The final corrected PC review passed all 134 runtime checks and produced 19 captures. The neck gap found in the close-up is corrected. The forced reduced-LOD portrait still shows hat-band/coat-detail simplification; full detail remains available on Android. Unreal reported one bind-pose warning and used its time-zero rebind fallback; the idle/walk/jog rendered captures were checked, but this is not a full animation or facial-deformation acceptance pass. Selected PC captures are in `Docs/Verification/ReedDrugstoreBenchmark`. Android BuildCookRun exited 0; the cook reported 0 errors and 0 warnings. APK signature verification passed, its certificate matches v0.1.3, and 16 KB ZIP alignment passed. Package version is 0.1.4-reed-drugstore, version code 5, ARM64. Size: 189,396,847 bytes. SHA-256: `27DDB3B157EB03FB0904EE4F1959104D45DD8B162B9E0F95E82DD8274BEFC791`. The APK is ready for physical-device testing; phone appearance, frame times and thermal behavior have not been measured.
