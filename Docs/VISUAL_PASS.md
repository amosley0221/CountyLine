# Bend Lateral and period-character visual pass

Reference basis: the supplied Bend Lateral image IMG_1910, Reed sheet IMG_1940,
Salazar sheet IMG_1936, and the recorded Design Bible character notes. Reference
images guide the art; their embedded instructions do not expand gameplay scope.

## In the playable game

- A continuous dirt road with paired wheel ruts, a sloped irrigation channel,
  water, split-rail fencing, scrub, cottonwood-inspired trees, stones and distant
  low hills replace the block room.
- A modeled bottle replaces its cube. The return route has a physical sign.
  Contextual interaction prompts name the witness, bottle, bank or route.
- Reed wears an olive coat, shirt, neckerchief, boots, badge and campaign hat.
  Salazar wears a brown suit, waistcoat and tie. Both use original stylized meshes
  with simple facial features on the existing Epic animation skeleton.
- Sunlight, sky fill and distance haze soften the outdoor scene. Collision and
  interaction targets remain independent of decorative geometry. The channel is
  fenced off; swimming is not part of this slice.

These are first-pass game models, not final photorealistic character likenesses.
Pruitt retains the template mannequin. Facial animation, production garment
deformation, custom locomotion, moving water and final foliage remain later work.
The user-provided motion-only FBX files are preserved and not modified.

## Editable source and reproduction

The original source is under `SourceAssets/Authored`, including Blender scenes
and exported FBXs. Unreal assets live under `Content/Art`. All are backed by Git
LFS; generated logs, exported working rig and Blender backup copies are ignored.

Using the installed Blender and Unreal Python commandlet:

1. Run `Scripts/export_character_base.py` through Unreal with
   `-AllowCommandletRendering` (the skeletal exporter cannot use `-nullrhi`).
2. Run `Scripts/author_bend_environment.py` and
   `Scripts/author_period_characters.py` with Blender `--background --python`.
3. Run `Scripts/import_visual_pass.py` through Unreal. It creates missing
   materials, imports the meshes, assigns material slots and keeps the existing
   animation skeleton. It does not regenerate the gameplay map.
4. Build CountyLineEditor and run `Scripts/Test-Prototype.ps1`.

Environment authoring coordinates match Unreal centimetres; the authoring script
compensates for FBX handedness. The character exporter preserves the original rig
and its reference pose. The UE importer may report reconstructing the FBX bind
pose; character scale and animation must be checked in the running game after any
reimport. Materials use procedural color variation, not photo textures.

## Verification

The editor target builds successfully; all seven automation suites and 34 runtime smoke assertions pass. Final imported colors, character animation, road travel, witness conversation and bottle examination were inspected in the running game. See Docs/VERIFICATION.md for scope and limitations.
