# Street ground and Reed refinement

This pass continues the finished-street benchmark; it does not declare the street or cast final. No final campaign missions were added.

## Street

The previous masked road material could expose holes and look like a raised sheet over the ground. `M_CountyGroundV3` is opaque and paints road coverage in world coordinates. Town ground, residential ground, road overlays and the county approach share that finish. Intersections use the union of road regions, with a soft soil-to-dust transition and restrained grain. Decorative road slabs sit just above the collision ground and no longer cast tiny edge shadows. Building footprints and ground collision are unchanged.

`Scripts/build_street_ground.py` contains the current road centers and extents, including four house paths. These must be updated alongside future geographic changes. This is a compact authored-layout solution, not the final county terrain/road system. The material uses no translucency, runtime virtual textures, tessellation or downloaded textures. It still needs physical-device performance review after each substantial expansion.

## Reed

The supplied IMG_1940.JPG remains the reference. This iteration adjusts the sleeve-to-shoulder join, constructs palms along the rig's wrist-to-knuckle axes, reduces the protrusion of eyes/brows/nose, reshapes the jaw, and replaces the long tie shape with two short neckerchief ends. The coat palette, badge, campaign hat, skeleton and locomotion remain in place. Only Reed is reauthored; the civilian and deputy stand-ins are not silently changed.

This is still procedural stylized prototype art. It is not a finished likeness, facial-animation rig, realistic skin/hair treatment, or final clothing deformation. Establishing that higher quality standard for Reed and a civilian remains part of the larger milestone.

## Reproduction and review

1. Run Blender with `Scripts/author_period_characters.py -- --reed-only`.
2. Run `Scripts/import_street_cast_refinement.py` through Unreal Python.
3. Build the PC editor; run `Scripts/Test-Prototype.ps1`.
4. Run the rendered smoke review with `-CLSmokeTest -CLTownReview -CLStreetReview`. It now captures 15 views, including GroundTransition and ReedPortrait.
5. Build Android with `Scripts/Build-Android.ps1`. Version 0.1.2-street-cast uses version code 3.

The first screenshot review caught exposed sleeve caps after removing the old round shoulder shapes. The revised model extends the sleeves into the shoulder and spans the palm to the finger roots. Review evidence is retained separately from earlier benchmark screenshots.

The existing 20 Unreal automation suites, 10 map tests, and 134 runtime checks passed during this pass. Final screenshot and Android packaging results are appended after completion. Tests bypass the player's real save slot.

Final verification: the corrected mesh passed the 134-check rendered smoke run and 15-camera review with no material compilation errors. Android BuildCookRun exited 0; the cook reported 0 errors and 0 warnings. Version 0.1.2-street-cast (code 3) passes APK signature and 16 KB ZIP alignment checks, with the same signing certificate as 0.1.1. The APK is 184,684,315 bytes. SHA-256: 1F67639C750207F30FC098C9A6E0BE9FA930C98748BF36C46E6F51DDAFD94BCC. The new visual revision still needs user review on the phone; the earlier successful device acceptance belongs to 0.1.1.

Evidence: Docs/Verification/StreetCastRefinement. Local APK: F:/The County Line/The-County-Line-Android-v0.1.2.apk.

User acceptance: after installing v0.1.2, the user reported that its appearance looked fine. This is visual acceptance, not an instrumented performance measurement.
