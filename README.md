# The County Line

A PC, third-person game set in 1926–1928, built with Unreal Engine 5.8.

The supplied design proposes a narrative lawman simulation in fictional Rivas County, West Texas: Acting Sheriff Sam Reed investigates cases, writes reports, and navigates the county's competing interests. Losing the badge changes his access and livelihood while play continues in the same county.

## Current state

A jail-office gameplay prototype built for UE 5.8.2: third-person movement, Deputy Pruitt's introduction, report interaction, County Book, SIGN/HOLD choices, desk-only saving, and controller navigation throughout. The character and world art are placeholders. See `Docs/JAIL_PROTOTYPE.md` for controls and scope, and `Docs/VERIFICATION.md` for verification status.

## Open and build

1. Install Unreal Engine 5.8 and its supported Visual Studio C++ toolchain / Windows SDK. Install Git LFS and run `git lfs pull` after cloning to obtain binary assets.
2. Right-click `CountyLine.uproject` and generate Visual Studio project files.
3. Build `CountyLineEditor`, Development Editor, Win64, then open `CountyLine.uproject`.

Alternatively, in PowerShell, set `$EngineRoot` to your UE installation and run:

```powershell
& "$EngineRoot\Engine\Build\BatchFiles\Build.bat" CountyLineEditor Win64 Development "-Project=$PWD\CountyLine.uproject" -WaitMutex
```

The startup map is `/Game/Maps/L_JailOffice`. Press Play in Unreal, or run `Scripts/Play-Prototype.ps1` to launch an uncooked game window with your installed engine.

## Project documents

- `Docs/PRODUCTION_PLAN.md`: delivery stages and acceptance criteria.
- `Docs/DESIGN_BASELINE.md`: user requirements, handoff assumptions, and unresolved details.
- `Docs/COLLABORATION.md`: Codex / Claude Code responsibilities and handoff format.
- `Docs/JAIL_PROTOTYPE.md`: playable walkthrough, controls, and limitations.
- `Docs/ASSET_REGISTER.md`: mannequin, font, and supplied-animation provenance.
- `Docs/DESIGN_REVISION_NOTES.md`: differences introduced by the new Design Bible.
- `Docs/Reference/design_handoff_county_line/README.md`: original, unmodified reference overview.
- `Docs/Reference/design_handoff_county_line/boards/01-onesheet-system.dc.html`: start of the original linked boards.

The reference images are concept stand-ins. The three UI fonts and their licenses are included. Unreal assets and future FBXs use Git LFS; small reference PNGs use ordinary Git. No production Reed mesh or vehicle model is included yet.

The separate non-canon gameplay experiment is documented in [Docs/NONCANON_FREIGHT_TRIAL.md](Docs/NONCANON_FREIGHT_TRIAL.md). Launch it with the dedicated desktop shortcut or add -TestMission to Scripts/Play-Prototype.ps1. It never loads or writes campaign saves.
