# First walkable county route

Update: Court Street and Lang's boardinghouse now extend the western end into Pecos Bend. See `Docs/PECOS_BEND_TOWN.md`. The original road and investigation route remain connected.

The jail office and Bend Lateral now share a continuous, compact walking route. Leave the open office doorway, turn south onto the dirt road, and follow it east through the gap in the lateral's western fence. Return along the same road. Signs provide directions; their Book interaction does not teleport the player. Keyboard and controller movement are unchanged.

This is a roughly 100-metre prototype connection, not the final county scale, town plan, or World Partition implementation. The route uses existing original materials, simple ground and roadside geometry, and the authored Bend landscape. The channel remains inaccessible and swimming is not implemented. Vehicles, wider county exploration, and additional destinations remain future work.

## Discovery and recovery

Grounded entry into the jail office, county road, and Bend Lateral records `JailOffice`, `CountyRoad`, and `BendLateral` respectively. The Map page lists discovered places without a player dot or fast-travel menu. Repeated visits do not duplicate discoveries. Region changes update a fixed safe checkpoint rather than saving arbitrary positions every frame.

The case subsystem persists these fields with the report when the player writes the date at the desk. Discovery alone is not an autosave. Since the desk is currently the only save station, normal saved sessions resume at the office checkpoint. The current in-memory checkpoint also provides recovery if Reed falls below the route.

On startup, known checkpoint ids and matching transforms are restored once the pawn is available. Unknown ids or stale coordinates fall back to the office entrance. Existing saves without world-state fields use the normal initial spawn and discover the office when grounded. Runtime recovery preserves report decisions, evidence, and discoveries. Recovery does not write a save slot.

## Implementation and reproduction

`ACLCountyRoad` owns the static route, region classification, and checkpoint definitions. The player controller spawns it alongside Bend Lateral, tracks grounded visits, restores safe positions, and updates field ambience. The office wall has a real doorway and an open door leaf. Both the native Bend boundary and its visible fence have an entrance gap.

To reproduce the fence asset, run Blender with `Scripts/author_bend_environment.py -- --road-only`, then run `Scripts/import_road_fence.py` with Unreal's Python commandlet. This updates the editable environment scene and fence and ground exports, then imports only those two assets. Other imported character and scenery assets are preserved.

`TravelToBend` remains a developer smoke-test positioning helper for isolated investigation tests. Player-facing road directions never call it. Continuous-route smoke coverage sweeps the actual Reed capsule in small increments along the doorway and road in both directions, checking ground support and obstructions. Further checks cover discovery, checkpoint selection, recovery, invalid saved destinations, ambience, and unchanged case state.
