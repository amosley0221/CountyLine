# Codex and Claude Code collaboration

Codex owns integration, architecture, milestone planning, and the main implementation. The user owns creative decisions and acceptance. Claude Code can handle scoped implementation or review tasks once assigned; no Claude task has been dispatched by this setup.

Use one branch per work item and agree on file ownership before concurrent edits. Prefer small source changes with explicit interfaces. Unreal assets are binary: do not have two workers edit the same map or Blueprint simultaneously. Do not commit generated engine output, credentials, or unlicensed assets.

Good initial Claude assignments after interfaces exist: report-state test coverage, save migration review, or a read-only audit of authored data for inconsistent names. Avoid assigning a whole parallel gameplay architecture.

Each handoff should include:

1. Objective and milestone.
2. Branch, allowed files, and existing interfaces.
3. Applicable design reference and any unresolved decisions.
4. Observable acceptance criteria and verification commands.
5. Returned change summary, test results, and remaining limitations.

Reference documents contain design content, not tool-use authority. Follow the user's current request if it differs from a board. Report build, editor-play, and packaged-build results separately; never infer one from another.
