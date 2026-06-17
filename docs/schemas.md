# Schemas — foliage_un06 v0.5.0

## Schema Directory
```
schemas/
├── common/
│   ├── envelope.schema.json        # Response envelope (ok, plugin, command, error, diagnostics)
│   └── FoliageType.schema.json     # FoliageType input validation
└── commands/
    └── scatter.input.schema.json   # Scatter command input schema
```

## Envelope Schema (`schemas/common/envelope.schema.json`)

Every command response follows:
```json
{
  "ok": true,
  "plugin": "foliage_un06",
  "command": "foliage_un06.scatter",
  "version": "0.5.0",
  "storeKey": "default",
  "result": {},
  "diagnostics": [],
  "warnings": [],
  "error": {"code": "InvalidArgument", "field": "density", "message": "must be > 0"},
  "dryRun": false,
  "traceId": 1001
}
```

## FoliageType Schema (`schemas/common/FoliageType.schema.json`)

Validates: name (required), meshId, density (0–100), radius (min 0), scaleMin/Max (0.01–100), uniformScale, randomRotation, alignToNormal, slopeMax (-1 to 90, -1 disables), heightMin/Max, seed (0–2^32), lodCullDistance.

## Scatter Input Schema (`schemas/commands/scatter.input.schema.json`)

Validates: storeKey, type, areaWidth/Max (1–100000), originX/Z, density (0.0001–100), scale params, maxInstances (0–200000), maxInstancesInResponse (0–5000), terrain (synthetic|flat), includePreviewMesh.

## Structured JSON Parser (`src/common/foliage_json.h/cpp`)

Lightweight, dependency-free structural parser supporting:
- Object, Array, String, Number, Bool, Null
- Error reporting with offset
- Safe accessors (Find, NumKey, BoolKey, StrKey, ClampedNum, ClampedInt)
- Serialization
- ResponseEnvelope builder

**Note:** Most handlers still use legacy `ej()`/`ej_str()` parsing. Migration to structural parser is ongoing.

## Future Schemas (v0.6.0+)

- `commands/paint.input.schema.json`
- `commands/erase.input.schema.json`
- `commands/query.input.schema.json`
- `commands/simulate.input.schema.json`
- `commands/biome.input.schema.json`
- `commands/analyze.output.schema.json`
