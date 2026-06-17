# FoliageTypeV2 Data Model — foliage_un06

## Mapping: Unreal UFoliageType → Mont-ED FoliageTypeV2

| UE Category | UE Property | Mont-ED Field | Type |
|-------------|-------------|---------------|------|
| **Identity** | — | typeId, name, displayName, meshId | string |
| | | schemaVersion, debugColor | string |
| | | visible, locked | bool |
| **Placement** | Density | density | double |
| | Radius | radius | double |
| | DensityAdjustmentFactor | densityAdjustment | double |
| | Paint Density | paintDensity | double |
| | Erase Density | eraseDensity | double |
| **Scale** | Scaling (Uniform/Free/LockXY) | scalingMode | enum |
| | Scale X/Y/Z Min/Max | scaleXMin..scaleZMax | double×6 |
| **Rotation** | Random Yaw | randomRotation | bool |
| | Align to Normal | alignToNormal | bool |
| | Random Pitch Angle | rotationPitchMin/Max | double×2 |
| **Height/Slope** | Ground Slope (Min/Max) | slopeMin, slopeMax | double |
| | Height (Min/Max) | heightMin, heightMax | double |
| **Z Offset** | Z Offset (Min/Max) | zOffsetMin, zOffsetMax | double |
| **Layers** | Landscape Layers | inclusionLayers | string[] |
| | Exclusion Layers | exclusionLayers | string[] |
| | Min/Max Layer Weight | minLayerWeight, maxLayerWeight | double |
| **Determinism** | Distribution Seed | seed, distributionSeed | uint32 |
| **Procedural** | Num Steps | numSteps | int |
| | Seeds Per Step | seedsPerStep | int |
| | Initial Seed Density | initialSeedDensity | double |
| | Average Spread Distance | averageSpreadDist | double |
| | Spread Variance | spreadVariance | double |
| | Max Initial Age | maxInitialAge | double |
| | Max Age | maxAge | double |
| | Collision Radius | collisionRadius | double |
| | Shade Radius | shadeRadius | double |
| | Can Grow in Shade | canGrowInShade | bool |
| | Spawns in Shade | spawnsInShade | bool |
| | Overlap Priority | overlapPriority | double |
| **Rendering** | Start Cull Distance | cullStart | double |
| | End Cull Distance | cullEnd | double |
| | LOD Distances | lod0Distance, lod1Distance, lod2Distance | double |
| | Billboard Distance | billboardDist | double |
| | Use Impostor | useImpostor | bool |
| | Cast Shadow | castShadow | bool |
| **Wind** | Wind Strength | windStrength | double |
| | Sway Amount | swayAmount | double |
| | Leaf Flutter | leafFlutter | double |
| | Gust Frequency | gustFrequency | double |
| **Collision** | Collision Preset | collisionPreset | enum |
| | Collision with World | collisionWithWorld | bool |
| **Scalability** | Enable Density Scaling | densityScalingEnabled | bool |
| — | Importance Class | importanceClass | enum |

## Backward Compatibility

Old payloads using `scaleMin`/`scaleMax` (uniform) continue to work via `SetUniformScale()`.
New payloads can specify per-axis (`scaleXMin`/`scaleXMax`) with `scalingMode: "free"`.

## Example JSON (V2)

```json
{
  "name": "oak_tree",
  "meshId": "oak_mesh",
  "schemaVersion": "2.0",
  "scalingMode": "free",
  "scaleXMin": 0.8, "scaleXMax": 1.4,
  "scaleYMin": 0.9, "scaleYMax": 1.8,
  "scaleZMin": 0.8, "scaleZMax": 1.4,
  "slopeMin": 0, "slopeMax": 35,
  "heightMin": 0, "heightMax": 500,
  "importanceClass": 2,
  "lod0Distance": 500, "lod1Distance": 2000,
  "billboardDist": 8000, "useImpostor": true,
  "collisionPreset": 1,
  "windStrength": 1.2, "swayAmount": 0.6
}
```
