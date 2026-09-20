# Town blocks and public-space review

The user identified front-to-back shop crowding and benches aimed at the courthouse wall. This revision prioritizes a usable town plan over matching reference artwork. Images supply architectural character, not mandatory plot placement.

## Commercial blocks

Three businesses form the west block, side by side along Market Street: repairs to the south, the general store in the middle and dry goods to the north. Every frontage faces east. Shop bodies are 7 metres deep; their awnings and boardwalks face a 5-metre street rather than another shop's rear wall. A separate 3-metre service lane runs behind the block. Both streets connect the southern crossroad to North Lane. Side gaps between the shops remain open.

The pharmacy and post office form a second row east of Court Street, both facing west. Their entrances have boardwalks and a continuous public walking strip along Court Street. Their backs face open service space on the east edge. The jail retains a separate approach south of these shops.

Store geometry, signs and trim now share a per-building transform. Rotating a shop therefore rotates its door, lettering, awning and boardwalk together. This avoids a front-facing sign attached to a building whose actual approach faces elsewhere.

## Courthouse seating

The former six wall-facing side benches are replaced by four benches in two pairs. The southern pair face across the forecourt toward the crossroad; the northern pair face the open north green. Their backs face the courthouse. They stand roughly 6.5–7 metres from the main building's nearest wall, with the central entrance approach left clear.

## Continuity and map

The courthouse, jail, Lang's, homes, investigation route and saved checkpoint transforms retain their positions. The western ground/perimeter extends 10 metres to accommodate the commercial block and rear service lane. Court Street's discovery region expands to include these public areas; existing save ids stay valid.

`Scripts/draw_town_layout.py` reads each shop's yaw and calculates its rotated footprint. The regenerated [map](Maps/PecosBend-layout.svg) shows blue arrows for building fronts and green arrows for seating direction. The original reference images are retained unchanged. It remains a schematic of the current playable slice, not the final illustrated county map.

Runtime checks sweep Reed's capsule along both shopfront walks and the west service loop. Additional checks use the actual shop-door transforms to verify street-facing directions and clear approaches, and test the space in front of each bench. These supplement the existing report, save, residential and investigation checks. Street widths are authored layout dimensions, not a claim of tested vehicle navigation; vehicles and occupied shop interiors remain future work.
