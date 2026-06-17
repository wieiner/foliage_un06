# Performance — foliage_un06 v0.5.0

## Scatter Benchmark

Run via DLL or engine:
```powershell
# DLL
pwsh .\build_verify.ps1

# Engine
EditorApp.exe --bus-call foliage_un06.benchmark --payload '{"action":"run","areaWidth":100,"areaDepth":100,"density":0.05,"runs":5}' --allow-all-permissions --plugin-root "C:\\mont-ed-plg"
```

## Expected Performance (MSVC Release build)

| Scenario | Area | Density | Instances | Time (ms) | Inst/s |
|----------|------|---------|-----------|-----------|--------|
| Sparse trees | 500×500 | 0.005 | ~1,250 | ~3 | ~400 |
| Medium bushes | 200×200 | 0.04 | ~1,600 | ~2 | ~800 |
| Dense grass | 100×100 | 0.15 | ~1,500 | ~1.5 | ~1,000 |
| Biome scatter | 200×200 | 0.08 × 3 types | ~3,600 | ~6 | ~600 |

Ballpark: **500–1000 instances/ms** on typical hardware.

## Algorithm Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Poisson-disc scatter | O(N × k) | k = samples before rejection (default 30) |
| Spatial query (radius) | O(N_cell) | Grid-hash, ~9 cells per query |
| Spatial query (nearest) | O(N_cell) | Spiral search, max 49 cells |
| Cluster build | O(N × V) | V = mesh vertices per instance |
| Ecosystem compete | O(N × hits) | Spatial index limits per-cell checks |
| Height/slope filter | O(1) per instance | Finite-difference sampling |
| Noise modulation | O(N × octaves) | Perlin FBM at each instance |

## Memory

| Component | Per-instance |
|-----------|-------------|
| FoliageInstance (v1) | ~104 bytes (7 doubles + 2 bools + padding) |
| FoliageInstanceV2 | ~144 bytes (+ uint64 id, uint64 typeId, string name, age, etc.) |
| SpatialIndex cell | ~32 bytes (vector<int> overhead + avg 4 indices) |
| Cluster mesh vertex | ~48 bytes (6 doubles: pos + normal) |
| Cluster mesh index | ~4 bytes (uint32) |

## Limits Enforced

| Limit | Value |
|-------|-------|
| maxInstances (scatter) | 50,000 |
| maxBatchSize | 100 |
| maxCheckpoints per store | 50 |
| maxTerrainGridSamples | 2,500 (50×50) |
| maxInstancesInResponse | 5,000 |
| maxBrushPaintInstances | 5,000 |
| maxJSON per response | ~16KB buffer |

## Profiling Tips

1. Use `foliage_un06.benchmark` for timing
2. Monitor `rejected counts` in scatter response — high rejection = inefficient sampling
3. Large areas with low density: increase k (samples before rejection) for better fill
4. Small areas with high density: decrease k for speed
5. SpatialIndex cell size = 8.0 (adjustable in query.cpp)
6. PRNG is xoshiro256** (fast, no division operations)
