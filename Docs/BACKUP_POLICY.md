# GitHub backup

User preference: keep completed project work backed up to the CountyLine GitHub repository. Commit and push completed changes after appropriate verification. Do not commit another worker's in-progress edits without coordinating with that worker or the user.

The baseline prototype, source, tests, configuration, documentation, and Unreal assets are tracked. Git LFS stores Unreal binary assets and original source-asset backups.

`SourceAssets/Animations` preserves the user-supplied FBXs and animation archive. These source files are not imported game assets.

`SourceAssets/DesignBibleAndReferences` preserves the Design Bible, character/vehicle JPGs, and two video references.

`SourceAssets/Archives` preserves the two original reference ZIP downloads. The initial concept PNGs already live in `Docs/Reference/design_handoff_county_line/boards/uploads` and match the workspace originals.

Generated binaries, caches, local logs, personal saves, and credentials are excluded. They are not needed to recreate the project from source and the installed Unreal toolchain. GitHub backup is a source/asset backup, not a packaged game distribution.

After cloning, install Git LFS and run `git lfs pull` before opening Unreal. Files retained in the original workspace folders are untouched; the repository contains backup copies.
