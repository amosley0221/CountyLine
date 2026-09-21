# The Enterprise: carbon to public notice

The Enterprise is an open newspaper office beside Lang's, south of the square. Its north-facing entrance and outdoor notice board address the street. The room has Mara Holt's desk, a typewriter proxy and an original static character stand-in. Mara's role as editor and the signed-versus-held response come from the design bible; the dialogue, copy, geometry and character proxy are prototype-authored.

## Playthrough

Walk west from the jail to the square, then south to The Enterprise beside Lang's. Enter, approach Mara's desk and press E / controller A. An unsigned draft cannot be shown. With a signed or held report, choose **Show Mara the carbon**. Canceling before this action changes nothing.

A signed report produces a short posted account built only from `IncludedFacts` on the frozen carbon. Facts Reed omitted are absent. Notes learned after submission do not appear merely because Reed knows them now. A held report produces a **Story held** notice and withholds the death account. Repeated visits acknowledge the existing disposition. The notice can be read through Mara's conversation or from the board outside. Long copy has explicit next/previous pages, usable with keyboard and controller in either text style.

The Ledger records this first public consequence. **Write the date at the jail desk** to keep it. This is not automatic saving, a simulated print schedule, an NPC rumor system or a new numerical reputation effect. The original carbon, report disposition and separate inquiry remain unchanged.

## Persistence and compatibility

`FCLReportState::bEnterpriseReviewed` is an additive tagged property with a false default. Existing version-1 saves load without a newspaper handoff. `ShareWithEnterprise()` requires a read, signed or held report and rejects repeats. Validation rejects an enterprise handoff attached to a draft or unread report. Whole-struct save copying and dirty checking cover the new field; the save fixture includes it for submitted reports and permits its required default for drafts.

The notice text is derived from the submitted carbon, so only the handoff flag needs storing. Older executable builds ignore this new property and can drop it if they overwrite a newer save; the current build preserves it. No new location id or recovery point was introduced. The office remains in Court Street's existing region.

## Verification scope

The Enterprise automation suite covers signed/held outcomes, include/omit combinations, duplicate and invalid handoffs, later knowledge, frozen carbon, memory-only persistence and transactional validation rejection. Runtime checks sweep the entrance/desk/notice-board/return route, use the normal range/view/occlusion gates, cancel and perform handoffs with controller input, and inspect the actual world notice lettering. Test runs bypass the real save slot. Final art, voice acting, press machinery and full distribution remain future work.
