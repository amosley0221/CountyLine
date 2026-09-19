# Bend Lateral investigation presentation

The bottle and bank now open a close-up view alongside a small notebook panel.
Salazar's account uses the same presentation, keeping the witness visible and
animated. Recording a note still requires an explicit selection; looking,
orbiting, zooming or canceling never adds evidence or changes a signed carbon.

Controls: Q/E or LB/RB orbit; R/F or LT/RT zoom; the right stick also adjusts
the view. Clickable orbit/zoom buttons are available. The D-pad/left stick selects
controls, A/Enter activates, and B/Escape returns to third person. Orbit and zoom
are bounded so the view stays on the accessible side of the channel. Movement
is locked during inspection; the world, character motion and ambience continue.
The full County Book and pause menu retain their previous pause behavior.

## Character motion

Original six-second clips provide a relaxed breathing idle and a restrained
one-hand conversation gesture. They use the existing Epic skeleton. Reed's
locomotion blend space uses the new idle with the original movement samples;
speed input is smoothed during starts and stops. Salazar uses the conversation
clip only while his account is open, returning to idle when it closes.

These are prototype body animations, with no recorded speech or lip sync.
The user's motion-only FBXs remain unchanged and have not been retargeted.
Garment skinning and detailed hand/facial performance remain later art work.

## Sound

Original synthesized wind, channel water and sparse distant bird calls fade in
on field travel and fade out on return to the office. Water uses box-shaped
spatial attenuation along the channel. Short wood/dirt footstep sounds are
scheduled by actual grounded travel distance, with alternating pitch. Teleports,
standing still and pushing into walls do not accumulate footstep distance.

The WAVs are synthesized sources, not field recordings. They are a first sound
pass; a listening pass on the player's speakers/headphones is still useful.
No music, voice recordings or third-party sound libraries were added.

## Reproduction

- Run `Scripts/export_character_base.py` if the working Epic rig export is absent.
- Run `Scripts/author_field_motion.py` in Blender; it writes original animation
  FBXs and the editable Blender scene under `SourceAssets/Authored/Motion`.
- Run `Scripts/author_field_audio.py` with Python and NumPy; it writes deterministic
  WAVs under `SourceAssets/Authored/Audio`.
- Run `Scripts/import_field_immersion.py` through Unreal's Python commandlet;
  it imports `/Game/Art/Animations` and `/Game/Audio/Field` without rebuilding maps
  or changing the shared mannequin skeleton.
- The importer normalizes the in-place root track to identity. Animation-only
  FBX exports otherwise introduce a 100x root unit scale despite centimetre child
  tracks; this must not propagate to the runtime pose.
- Build CountyLineEditor and run `Scripts/Test-Prototype.ps1`.
