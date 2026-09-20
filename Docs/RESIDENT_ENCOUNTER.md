# North Lane resident encounter

The first residential encounter gives the approved town block a reason to visit: walk from the jail to the west house on North Lane, hear a River Road resident, choose whether to record the account, and return to the desk to write the date.

## Reference and authored scope

The supplied Design Bible, Rev. 1, calls for one River Road face in the vertical slice (page 18) and describes Act I as paper versus talk around the ditch (page 10). The resident remains unnamed. The North Lane placement and all dialogue in this encounter are prototype-authored; they are not quoted canon or an identification of the dead man's family. The resident says families use the bank path to reach fields, but did not see the man or witness the death and cannot place anyone there that morning. This establishes an attributed account, not a cause of death.

The character uses the existing period civilian skeletal mesh and field idle animation as temporary art, scaled to distinguish the silhouette slightly. It is a separate NPC, not Commissioner Salazar visiting the house. Unique likeness, voice, dialogue animation, schedules and an accessible house interior remain future work. No buildings or existing recovery checkpoints moved.

## Playthrough

1. Leave the jail, take the road west to Court Street, then follow Court Street north to North Lane. Turn west past the market row to the westernmost house.
2. Approach the resident by the front porch and look toward them. Press E / controller A.
3. Ask whether they saw what happened. Leaving either the greeting or answer records nothing.
4. Choose **Record the resident's account**. The account appears in Cases and People; the objective directs Reed back to the jail desk.
5. At the desk, a draft report can include or omit available field notes. If the original report was already signed or held, it remains frozen; the new account stays in the Book without changing that carbon or its follow-up disposition.
6. **Write the date** saves the account using the existing manual save. Leaving the game without doing so does not persist it.

Repeat conversations acknowledge the recorded account and cannot duplicate it. Keyboard Enter and controller A select choices; B / Escape leaves. Typed and handwritten copies contain the same words. The encounter uses the existing distance, view-angle and visibility trace gates, and the existing book input/pause ownership.

## State and regression coverage

The account is the `ResidentAccount` entry in the existing `FCLReportState::FieldNotes` array. There is no new save property or version. Existing whole-struct copying and dirty-state comparison include it. The resident does not unlock a new location id, change a checkpoint, or mutate the original signed/held carbon.

`CountyLine.Report.ResidentAccount` checks both dispositions, include/omit choices, receipt before/after submission, repeat recording, both copy preferences, and memory-only serialization through the production validator. Runtime smoke checks exercise the physical route, interaction gate, controller cancellation at both conversation stages, recording, repetition, out-of-range/look-away rejection, save round trip and return to the actual desk. Test runs bypass the player's real save slot.

Claude's town-layout coverage is integrated from `b6f8d1f`. The map generator now keys frontage by building identity rather than displayed sign text. A new sandboxed regression gives opposite-facing shops identical titles and checks each arrow's origin and direction. The generated drawing is unchanged for the current town.
