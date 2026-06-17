# TASK.md — Current Phase

## Phase 0 — Workspace Audit And Skeleton Smoke Test

Goal: prove that the generated workspace is coherent before implementing plugin features.

## Inputs

- Plugin name: `foliage_un06`
- Plugin type: `foliage_tool`
- Protocol preset: `core-bus`
- Agent: `generic`

## Required Actions

1. Inspect `plugin.monted.json` and confirm it parses as JSON.
2. Inspect `PROTOCOLS.md` and confirm it matches the manifest.
3. Inspect selected protocol source folders:
   - `src/core_abi` only when `core` is selected;
   - `src/bus` only when `bus` is selected;
   - `src/external` only when `external` is selected.
4. Run `pwsh ./build.ps1`.
5. Run `pwsh ./test.ps1`.
6. Create `docs/phase_report.md`.
7. Propose the exact Phase 1 file list, but do not implement Phase 1 yet.

## Acceptance Criteria

- Manifest exists and contains `name`, `type`, `protocolPreset` and `protocols`.
- Selected protocol files exist.
- Unsupported protocol folders are absent or explicitly documented as not selected.
- Build script completes.
- Test script completes.
- `docs/phase_report.md` includes risks and next minimal step.

## Non-Goals For Phase 0

- Do not implement the full plugin feature.
- Do not add dependencies.
- Do not create unselected protocol adapters.
- Do not redesign Mont-ED engine architecture.
