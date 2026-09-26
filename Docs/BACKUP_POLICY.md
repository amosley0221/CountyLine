# GitHub backup

User preference: keep completed project work backed up to the CountyLine GitHub repository. Commit and push completed changes after appropriate verification. Do not commit another worker's in-progress edits without coordinating with that worker or the user.

On September 25, 2026, the user explicitly approved publishing the Android v0.1.2 APK, matching source, authored assets, and review images to the public CountyLine repository, and gave standing approval for future CountyLine updates. Publish completed, verified project changes and Android prototype pre-releases within that scope. This does not authorize unrelated uploads or override tool permission checks.

The baseline prototype, source, tests, configuration, documentation, and Unreal assets are tracked. Git LFS stores Unreal binary assets and original source-asset backups.

`SourceAssets/Animations` preserves the user-supplied FBXs and animation archive. These source files are not imported game assets.

`SourceAssets/DesignBibleAndReferences` preserves the Design Bible, character/vehicle JPGs, and two video references.

`SourceAssets/Archives` preserves the two original reference ZIP downloads. The initial concept PNGs already live in `Docs/Reference/design_handoff_county_line/boards/uploads` and match the workspace originals.

Generated binaries, caches, local logs, personal saves, and credentials are excluded from source control. They are not needed to recreate the project from source and the installed Unreal toolchain. Distributable Android APKs are published separately as GitHub Release assets; release tags identify the source commit used to build them.

After cloning, install Git LFS and run `git lfs pull` before opening Unreal. Files retained in the original workspace folders are untouched; the repository contains backup copies.
