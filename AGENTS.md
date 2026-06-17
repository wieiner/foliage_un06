# AGENTS.md — foliage_un06

You are working inside a generated Mont-ED plugin workspace for a private Unreal-like engine.

- Name: `foliage_un06`
- Type: `foliage_tool`
- Protocol preset: `core-bus`
- Agent profile: `generic`

## Operating Contract

1. Read `TASK.md`, `PLUGIN_SPEC.md`, `PROTOCOLS.md`, `plugin.monted.json` and this file before editing.
2. Implement only the current phase from `TASK.md`.
3. Keep the workspace buildable after every phase.
4. Run `pwsh ./build.ps1` and `pwsh ./test.ps1` before reporting success.
5. Write or update `docs/phase_report.md` after each phase.
6. Explain any skipped check explicitly.
7. Do not edit the sibling prompt sidecar unless the task is specifically about prompts.
8. Do not add unrelated dependencies, engines, UI frameworks or build systems.

## Protocol Boundaries

Selected preset:

```text
core-bus
```

Selected protocol array:

```json
["core","bus"]
```

Rules:

- If `core` is not selected, do not create native ABI entrypoints.
- If `bus` is not selected, do not create message bus handlers.
- If `external` is not selected, do not create MCP/OpenRouter/JSON-RPC tools.
- Shared plugin logic belongs behind adapters, not inside transport boilerplate.
- Manifest, source folders, docs and tests must agree about selected protocols.

## Native ABI Rules

For `core`:

- use C ABI exports or versioned function tables;
- use opaque handles;
- do not pass STL containers, exceptions or CRT-owned memory across DLL boundaries;
- validate version numbers and struct sizes;
- keep ownership and lifetime explicit.

## Message Bus Rules

For `bus`:

- define stable command ids;
- validate typed payloads;
- include trace/correlation ids where practical;
- document threading and transaction behavior;
- gate unsafe commands behind explicit permissions.

## External Tool Rules

For `external`:

- provide JSON Schema for every public tool/resource;
- default destructive operations to dry-run;
- document file, network and process permissions;
- return structured errors;
- keep hot-path engine operations out of JSON.

## Required Phase Report

Create or update:

```text
docs/phase_report.md
```

Use this structure:

```text
# Phase Report

## Summary
## Files Read
## Files Changed
## Protocol Surfaces Touched
## Commands Run
## Validation Result
## Risks And Open Questions
## Next Minimal Step
```

Do not claim the phase is complete if build/test were not run or if a selected protocol surface is missing.
