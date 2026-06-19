# Technical Debt Remediation Plan — foliage_un06 v0.6.0

## Audit Findings Summary

| Severity | Count | Categories |
|----------|-------|------------|
| 🔴 Critical | 13 | Buffer overflow, exception safety, data races, command count mismatch, stub handlers, destructive flags, file security |
| 🟠 High | 7 | Missing includes, thread safety, build scripts, test coverage |
| 🟡 Medium | 20 | C-style casts, magic numbers, dead code, error handling |
| 🟢 Low | 11 | Comments, formatting, naming |

## Fix Order

### Phase 1: Critical (13 issues)
1. Add buffer bounds guards to all unbounded `out[pos++]` calls
2. Fix exception safety across DLL boundary
3. Fix file path traversal in export/import/CSV
4. Fix data races: static g_biomes, ensureDefaultTypes mutex
5. Sync command count: registry 53→63, inspect 17→63, health_check 41→63, list_commands 41→63
6. Fix stub handlers: wind_params, collision_settings, lod_config, cull_distance, z_offset, layer_mask, auto_lod — store state in g_types registry
7. Fix destructive flags: reseed, jitter, snap, transform, random_replace, filter_by_type, layer_mask, noise_modulate, z_offset, mirror_region, merge

### Phase 2: High (7 issues)
8. Fix missing #include <cstdio> in foliage_json.h
9. Fix missing #include <cstring> in foliage_command_registry.h
10. Fix thread-unsafe static in foliage_json.cpp::StrOr
11. Update test.ps1 to check all 18 common files
12. Fix build.ps1 hardcoded paths, add /WX
13. Fix verify_build.ps1 inconsistent CRT and error handling

### Phase 3: Medium (20 issues)
14. Replace C-style casts with static_cast (11 in pattern.cpp, 2 in json.cpp/h, 1 in biome.cpp)
15. Remove dead code: SplinePath, PaintBrush unused param
16. Fix magic numbers with named constants
17. Fix build.ps1 source file list duplication
18. Fix dll_verify.cpp hardcoded path
19. Fix GUI version string

### Phase 4: Low (11 issues)
20. Add explanatory comments for magic constants
21. Fix manifest formatting inconsistency
22. Remove unused permissions from top-level list
