# Street and supporting cast pass

Continues the finished-street milestone after the user accepted Android v0.1.2's appearance. This is an incremental visual pass, not final likeness or finished county art.

## Changes

- Five shops now have distinct opaque painted doors, frames, lower panels and signboards: sage drugstore, ochre dry goods, oxide grocer, slate post office and umber repair shop. Restrained world-space grain avoids additional textures or transparency.
- The grocer has a flour/coffee board, dry goods has window lettering, and the repair shop has a tool rail. Props stay inside the existing frontage; public routes and collision envelopes are unchanged.
- Daylight is less yellow, sky fill is slightly reduced, and the directional shadow range is bounded to 180 metres with three cascades on PC; Android retains its existing one-cascade cap. Display fill is reduced. Physical-device cost and appearance still need checking; no measured frame-rate improvement is claimed.
- Pruitt receives a dedicated tan shirt/trouser model with pockets, tie, badge and swept hair, following the silhouette and palette of IMG_1934.JPG. It uses the existing skeleton and idle animation. It is not a photorealistic likeness or a historical uniform certification.
- The unnamed North Lane resident receives a separate shirt, waistcoat and flat-cap model, replacing the reused Salazar mesh. This is a provisional civilian design, not a new named cast member. Reed and Salazar remain unchanged.

## Reproduction

1. Run Blender with `Scripts/author_supporting_cast.py`. It reuses rig-authoring helpers without exporting Reed or Salazar.
2. Run `Scripts/import_street_supporting_cast.py` through Unreal Python.
3. Build the editor and run `Scripts/Test-Prototype.ps1`.
4. Run the rendered smoke review with `-CLSmokeTest -CLTownReview -CLStreetReview`. Its 16 views include Pruitt in the office and the North Lane resident.
5. Package with `Scripts/Build-Android.ps1`. Android v0.1.3 uses version code 4 and the existing application ID/signing identity.

No campaign missions, save-format changes or cloud synchronization are included. Models remain procedural stylized art; facial rigging, final skin/hair treatment and final cast approval are future work.

## Verification

The PC editor build passed. All 20 Unreal automation suites, 10 map checks and 134 runtime smoke checks passed. Screenshot review caught pointed sleeve caps on the new models; the sleeve starts were moved farther into the torso and the trouser pelvis/waistcoat silhouettes tightened. Pruitt's review camera was moved to show his front. The final rendered review passed all 134 runtime checks and produced 16 captures without material compilation errors. Selected PC-rendered evidence is in `Docs/Verification/StreetSupportingCast`. Shoulder weights now blend the inner sleeve ends into the torso; the silhouettes remain visibly angular and are not final clothing deformation. Android BuildCookRun exited 0; cooking reported 0 errors and 0 warnings. The APK signature matches v0.1.2 and 16 KB ZIP alignment verification passed. Package version code is 4; size is 184,822,867 bytes. SHA-256: `86F4136E4F8A85365044E99A87D4CC9D1CD501127374E956B278DDD8DC627CA6`. Physical-device validation of v0.1.3 is still pending.
