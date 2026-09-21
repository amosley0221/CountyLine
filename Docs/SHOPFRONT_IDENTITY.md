# Shopfront identity and Reed appearance, first pass

The approved block layout is unchanged. Closed shops now have two recessed display bays behind lightly tinted glazing, with warm display lights and neutral backing. The full building collision envelope remains invisible and intact, so displays do not accidentally become entrances. Visible masonry is split around those bays.

- General store: produce display and one stock barrel.
- Drugstore: labeled bottles and stoppers on display shelves.
- Dry goods: stacked folded fabrics.
- Post office: tied parcels and a wall-mounted letter box.
- Repair shop: displayed hand tools and an outdoor workbench with vise.

Road surfaces have irregular terrain reveals along their boundaries and restrained color wear. Court Street paving has small joint/height variations without introducing blocking collision. This remains procedural prototype environment art; full authored dirt transitions and close-up shop interiors are future work.

Reed's dedicated first appearance pass uses IMG_1940.JPG for the olive-brown coat, tan shirt, brown trousers, leather boots, and pale metal badge. Added pocket flaps, pocket seams, belt buckle, boot soles and lacing preserve the existing animation skeleton. Dedicated materials remove the coarse mottling from his principal surfaces without recoloring the civilian cast. The campaign hat remains worn for gameplay. His face, hair, hands and garment deformation are still stylized placeholders, not the final reference likeness.

Reproduction:
1. Run Blender with Scripts/author_period_characters.py -- --reed-only.
2. Run Scripts/import_reed_appearance.py in Unreal Python.
3. Run Scripts/build_shop_details.py in Unreal Python to bind Reed's dedicated materials and generate missing shop materials.
4. Build CountyLineEditor and run Scripts/Test-Prototype.ps1.

Generated material assets are immutable: create a new revision for shader changes rather than deleting expressions on materials loaded by actor defaults. The Reed-only path writes ReedAppearance.blend and never reauthors Salazar.

Verification includes the existing closed-shop footprint and approach checks, report/save/controller regressions, and offscreen screenshots. The visual fixture now adds DrugstoreDisplay, DryGoodsDisplay and ReedAppearance to the ten town views. Its player placement is restricted to the save-isolated smoke fixture.

Validated: Win64 Development editor build succeeded; all 20 Unreal automation suites and 10 Python map tests passed; 134 runtime checks passed. The final offscreen review exited 0 with no material compilation errors. Screenshot evidence is under Docs/Verification/PecosBend. Reed's FBX import reconstructed its bind pose successfully; runtime skeleton and animation checks passed.
