# Protocols — foliage_un06

Selected preset: `core-bus`

Selected protocol array:

```json
["core","bus"]
```

## Protocol Meanings

### `core`

Native ABI fast path for trusted Mont-ED runtime/editor integration.

Use it for hot-path or low-latency operations. The adapter must use C ABI exports, versioned structs or function tables, explicit ownership, opaque handles and stable error codes. It must not pass C++ STL containers, exceptions or allocator-owned memory across DLL boundaries.

### `bus`

In-process message bus for editor/runtime command handling.

Use it for tool commands, editor actions, transactions, diagnostics and integration points that need traceability. The adapter must define stable command ids, typed payloads, validation, permission gates, trace ids and threading expectations.

### `external`

JSON-RPC/MCP/OpenRouter-shaped external profile.

Use it for agent-facing tools, resources and prompts. The adapter must provide schemas, structured errors, dry-run for destructive actions, permission documentation and safe file/network/process boundaries.

## Adapter Architecture

The protocol adapters should call shared plugin logic. Do not implement three separate plugins inside three protocol folders.

Recommended shape:

```text
src/
|-- core_abi/       # C ABI adapter, if selected
|-- bus/            # message bus adapter, if selected
|-- external/       # JSON schema/tool descriptor, if selected
`-- common/         # shared plugin logic, when Phase 1 starts
```

Phase 0 may not include `src/common` yet. Add it only when a real capability is implemented.

## Consistency Checklist

- `plugin.monted.json.protocolPreset` equals `core-bus`.
- `plugin.monted.json.protocols` equals the selected protocol array above.
- Selected protocol source folders exist.
- Unsupported protocol source folders are absent or intentionally documented.
- `build.ps1` and `test.ps1` validate these assumptions.
- `docs/phase_report.md` records any mismatch.

## Capability Mapping Template

When Phase 1 begins, fill this table in `PLUGIN_SPEC.md` or `docs/phase_report.md`:

```text
capability | shared service | core export | bus command | external tool/resource | notes
```

Rows for unselected protocols should say `not selected`, not `todo`.
