# Руководство пользователя — foliage_un06 v0.6.1

## Что это такое

**foliage_un06** — плагин для Mont-ED, который делает Foliage Scatter (разброс растительности). Это аналог Foliage Mode из Unreal Engine: вы задаёте параметры травы/кустов/деревьев, seed, плотность, фильтры, и плагин детерминированно размещает тысячи инстансов на terrain.

**63 команды**, 10 категорий: scatter, paint, filter, analysis, patterns, biomes, persistence, configuration, performance, utility.

---

## Быстрый старт — 3 шага

### Шаг 1. Собрать плагин

Откройте PowerShell и выполните:
```powershell
cd C:\mont-ed-plg\foliage_un06
pwsh .\build.ps1
```
Вы увидите:
```
============================================
[build] foliage_un06 v0.5.0 (foliage scatter plugin)
============================================
[1/3] Building foliage_un06.dll ...
  -> C:\mont-ed-plg\foliage_un06\build\foliage_un06.dll
[2/3] Building foliage_test_gui.exe ...
  -> C:\mont-ed-plg\foliage_un06\build\foliage_test_gui.exe
[3/3] Building golden_test.exe ...
=== Golden Tests: 25 passed, 0 failed ===
```

### Шаг 2. Запустить Standalone GUI

```powershell
cd C:\mont-ed-plg\foliage_un06
.\build\foliage_test_gui.exe
```

**Что вы видите в окне:**
- **Левая панель** — поля для ввода параметров:
  - `Area Width` / `Area Depth` — размер области в world units
  - `Density` — плотность (0.04 = средняя, 0.15 = густая трава)
  - `Scale Min` / `Scale Max` — разброс масштаба
  - `Slope Max (°)` — максимальный уклон (35 = не ставить на крутых склонах)
  - `Height Min` / `Height Max` — диапазон высот
  - `Seed` — зерно случайности (меняйте для разных вариантов)
  - Чекбоксы: `Align to Normal` (инстансы следуют рельефу), `Random Rotation`
  - Радио-кнопки: Hills / Flat (тип terrain), Cross / Cube (тип mesh)
- **Правая панель** — 2D вид сверху, инстансы отрисованы точками:
  - Цвет = высота terrain (зелёный → жёлтый → красный)
  - Белая линия = направление yaw
  - Зелёный круг = кисть Paint
  - Красный круг = кисть Erase
- **Снизу** — статус-бар с информацией

**Горячие клавиши:**
| Клавиша | Действие |
|---------|----------|
| **F5** | Scatter — разбросать инстансы |
| **F6** | Paint — нарисовать кистью в позиции курсора |
| **F7** | Toggle Erase — включить/выключить режим стирания |
| **F8** | Clear — очистить всё |
| **F9** | Export JSON — сохранить результат в файл |
| **+/-** | Zoom — приблизить/отдалить |
| **Стрелки** | Pan — перемещение по карте |
| **Ctrl+R** | Reset view — сбросить вид |
| **Ctrl+↑/↓** | Смена seed (быстрый перебор вариантов) |
| **Клик на canvas** | Переместить кисть в точку клика |

### Шаг 3. Посмотреть результат в движке Mont-ED

```powershell
cd C:\mont-ed
$ENGINE = ".\build-codex-new\Debug\EditorApp.exe"
$ROOT = "--plugin-root C:\\mont-ed-plg"

# Проверить плагин
& $ENGINE --bus-call foliage_un06.inspect --payload "{}" --allow-all-permissions $ROOT

# Разбросать траву 200×200
& $ENGINE --bus-call foliage_un06.scatter --payload '{"storeKey":"demo","areaWidth":200,"areaDepth":200,"density":0.03,"seed":42,"includePreviewMesh":true}' --allow-all-permissions $ROOT

# Анализ
& $ENGINE --bus-call foliage_un06.analyze --payload '{"storeKey":"demo","areaWidth":200,"areaDepth":200}' --allow-all-permissions $ROOT
```

---

## Все 63 команды — категории и примеры

### Core (основные)
```powershell
# Информация о плагине
& $ENGINE --bus-call foliage_un06.inspect --payload "{}" --allow-all-permissions $ROOT

# Список типов растительности
& $ENGINE --bus-call foliage_un06.get_types --payload "{}" --allow-all-permissions $ROOT

# Добавить свой тип
& $ENGINE --bus-call foliage_un06.add_type --payload '{"name":"my_grass","density":0.12,"scaleMin":0.3,"scaleMax":1.0,"seed":123,"meshId":"cross_billboard"}' --allow-all-permissions $ROOT
```

### Scatter (разброс)
```powershell
# Базовый разброс
& $ENGINE --bus-call foliage_un06.scatter --payload '{"storeKey":"my_scene","areaWidth":300,"areaDepth":300,"density":0.04,"seed":42}' --allow-all-permissions $ROOT

# Multi-type: трава 60% + кусты 30% + деревья 10%
& $ENGINE --bus-call foliage_un06.scatter_multi_type --payload '{"storeKey":"forest","types":[{"name":"grass_default","weight":0.6},{"name":"bush_default","weight":0.3},{"name":"tree_pine","weight":0.1}],"areaWidth":500,"areaDepth":500,"seed":42}' --allow-all-permissions $ROOT

# Автозаполнение (fill)
& $ENGINE --bus-call foliage_un06.fill --payload '{"storeKey":"my_scene","areaWidth":200,"areaDepth":200,"passes":3}' --allow-all-permissions $ROOT

# Экосистема (simulate)
& $ENGINE --bus-call foliage_un06.simulate --payload '{"storeKey":"my_scene","collisionRadius":8,"numSteps":3,"removeLosers":true}' --allow-all-permissions $ROOT
```

### Paint (рисование)
```powershell
# Кисть — нарисовать
& $ENGINE --bus-call foliage_un06.paint --payload '{"storeKey":"my_scene","centerX":100,"centerZ":100,"radius":25,"densityMul":2.0}' --allow-all-permissions $ROOT

# Кисть — стереть
& $ENGINE --bus-call foliage_un06.erase --payload '{"storeKey":"my_scene","centerX":100,"centerZ":100,"radius":15}' --allow-all-permissions $ROOT

# Разные формы кисти: donut (кольцо)
& $ENGINE --bus-call foliage_un06.brush_shapes --payload '{"shape":"donut","cx":100,"cz":100,"radius":30,"innerRadius":15}' --allow-all-permissions $ROOT

# Точное размещение одного инстанса
& $ENGINE --bus-call foliage_un06.place_single --payload '{"x":150,"z":150,"type":"grass_default"}' --allow-all-permissions $ROOT
```

### Patterns (паттерны)
```powershell
# Радиальное размещение (круги/кольца)
& $ENGINE --bus-call foliage_un06.radial --payload '{"centerX":200,"centerZ":200,"outerRadius":80,"fillDisc":true}' --allow-all-permissions $ROOT

# Вдоль пути
& $ENGINE --bus-call foliage_un06.along_path --payload '{"pathX":[0,100,200],"pathZ":[0,100,200],"width":10,"density":0.2,"parallel":true}' --allow-all-permissions $ROOT

# Естественные кластеры (Thomas cluster)
& $ENGINE --bus-call foliage_un06.clump --payload '{"parentCount":8,"clusterRadius":20,"childrenPerParent":10,"areaWidth":200,"areaDepth":200}' --allow-all-permissions $ROOT
```

### Analysis (анализ)
```powershell
# Статистика
& $ENGINE --bus-call foliage_un06.analyze --payload '{"storeKey":"my_scene","areaWidth":200,"areaDepth":200}' --allow-all-permissions $ROOT

# Тепловая карта плотности
& $ENGINE --bus-call foliage_un06.density_heatmap --payload '{"storeKey":"my_scene","areaWidth":200,"areaDepth":200,"cellSize":10}' --allow-all-permissions $ROOT

# Покрытие (% площади)
& $ENGINE --bus-call foliage_un06.coverage_analysis --payload '{"storeKey":"my_scene","cellSize":10}' --allow-all-permissions $ROOT

# Границы и размеры
& $ENGINE --bus-call foliage_un06.compute_bounds --payload '{"storeKey":"my_scene"}' --allow-all-permissions $ROOT

# Группировка по кластерам
& $ENGINE --bus-call foliage_un06.group_by_cluster --payload '{"storeKey":"my_scene","clusterSize":16}' --allow-all-permissions $ROOT

# Пространственный запрос (nearest)
& $ENGINE --bus-call foliage_un06.query --payload '{"storeKey":"my_scene","mode":"nearest","x":100,"z":100,"radius":50}' --allow-all-permissions $ROOT
```

### Persistence (сохранение)
```powershell
# Экспорт в JSON
& $ENGINE --bus-call foliage_un06.export_state --payload '{"storeKey":"my_scene","path":"build/my_forest.json"}' --allow-all-permissions $ROOT

# Импорт из JSON
& $ENGINE --bus-call foliage_un06.import_state --payload '{"storeKey":"my_scene","path":"build/my_forest.json"}' --allow-all-permissions $ROOT

# Экспорт CSV для Blender
& $ENGINE --bus-call foliage_un06.export_instances_csv --payload '{"storeKey":"my_scene","path":"build/my_forest.csv","maxRows":5000}' --allow-all-permissions $ROOT

# Копировать store
& $ENGINE --bus-call foliage_un06.duplicate_store --payload '{"sourceKey":"my_scene","targetKey":"my_scene_backup"}' --allow-all-permissions $ROOT

# Сравнить две версии
& $ENGINE --bus-call foliage_un06.diff_stores --payload '{"storeA":"my_scene","storeB":"my_scene_backup"}' --allow-all-permissions $ROOT

# Очистить store (dry-run — посмотреть что удалится)
& $ENGINE --bus-call foliage_un06.clear_store --payload '{"storeKey":"test_temp","dryRun":true}' --allow-all-permissions $ROOT
```

### Filters (фильтры)
```powershell
# Шум Perlin — естественная вариация плотности
& $ENGINE --bus-call foliage_un06.noise_modulate --payload '{"storeKey":"my_scene","noiseScale":40,"noiseThreshold":0.4,"noiseOctaves":3}' --allow-all-permissions $ROOT

# Маски слоёв
& $ENGINE --bus-call foliage_un06.layer_mask --payload '{"action":"set","type":"grass_default","inclusionLayers":"grass,meadow","exclusionLayers":"water,rock"}' --allow-all-permissions $ROOT

# Затухание к краям (density falloff)
& $ENGINE --bus-call foliage_un06.density_falloff --payload '{"storeKey":"my_scene","areaWidth":200,"areaDepth":200,"falloffWidth":20,"falloffType":"smoothstep"}' --allow-all-permissions $ROOT

# Зоны исключения (блокирующие volume)
& $ENGINE --bus-call foliage_un06.exclusion_zone --payload '{"action":"add","name":"no_trees_zone","shape":"circle","circleX":100,"circleZ":100,"circleRadius":30}' --allow-all-permissions $ROOT

# Удаление выбросов (outliers)
& $ENGINE --bus-call foliage_un06.remove_outliers --payload '{"storeKey":"my_scene","sigmaThreshold":2.5}' --allow-all-permissions $ROOT

# Jitter — случайное смещение позиций
& $ENGINE --bus-call foliage_un06.jitter_positions --payload '{"storeKey":"my_scene","maxJitter":3,"seed":99}' --allow-all-permissions $ROOT

# Snap — привязать к terrain
& $ENGINE --bus-call foliage_un06.snap_to_terrain --payload '{"storeKey":"my_scene"}' --allow-all-permissions $ROOT

# Reseed — новый seed для масштабов и поворотов
& $ENGINE --bus-call foliage_un06.reseed --payload '{"storeKey":"my_scene","seed":777}' --allow-all-permissions $ROOT
```

### Configuration (настройки)
```powershell
# Ветер
& $ENGINE --bus-call foliage_un06.wind_params --payload '{"action":"set","type":"grass_default","windStrength":1.2,"swayAmount":0.7}' --allow-all-permissions $ROOT

# Коллизии
& $ENGINE --bus-call foliage_un06.collision_settings --payload '{"action":"set","type":"tree_pine","collisionEnabled":true,"collisionPreset":"BlockAll"}' --allow-all-permissions $ROOT

# LOD дистанции
& $ENGINE --bus-call foliage_un06.lod_config --payload '{"action":"set","type":"grass_default","lod0Dist":200,"lod1Dist":800,"lod2Dist":2000,"billboardDist":4000,"useImpostors":true}' --allow-all-permissions $ROOT

# Cull distance
& $ENGINE --bus-call foliage_un06.cull_distance --payload '{"action":"set","type":"tree_pine","startCullDistance":100,"endCullDistance":20000}' --allow-all-permissions $ROOT

# Z offset
& $ENGINE --bus-call foliage_un06.z_offset --payload '{"action":"set","type":"grass_default","zOffsetMin":-2,"zOffsetMax":5}' --allow-all-permissions $ROOT

# Auto LOD (рекомендации)
& $ENGINE --bus-call foliage_un06.auto_lod --payload '{"importanceClass":"tree"}' --allow-all-permissions $ROOT
# → {"lod0":600,"lod1":2500,"lod2":5000,"billboard":12000,"startCull":100,"endCull":25000}
```

### Performance (производительность)
```powershell
# Benchmark
& $ENGINE --bus-call foliage_un06.benchmark --payload '{"action":"run","areaWidth":200,"areaDepth":200,"density":0.05,"runs":5}' --allow-all-permissions $ROOT

# Оценка памяти
& $ENGINE --bus-call foliage_un06.store_memory --payload '{"storeKey":"my_scene"}' --allow-all-permissions $ROOT
```

### Utility (утилиты)
```powershell
# Pipeline: scatter → noise → jitter → snap в одном вызове
& $ENGINE --bus-call foliage_un06.pipeline --payload '{"storeKey":"auto_forest","cmd_0":"scatter","payload_0":"{\"areaWidth\":200,\"areaDepth\":200,\"density\":0.05,\"seed\":42}","cmd_1":"noise_modulate","payload_1":"{\"noiseScale\":40,\"noiseThreshold\":0.3}"}' --allow-all-permissions $ROOT

# Здоровье плагина
& $ENGINE --bus-call foliage_un06.health_check --payload "{}" --allow-all-permissions $ROOT

# Список всех команд
& $ENGINE --bus-call foliage_un06.list_commands --payload '{"compact":true}' --allow-all-permissions $ROOT

# Валидация payload без выполнения
& $ENGINE --bus-call foliage_un06.validate_payload --payload '{"commandId":"scatter","payload":"{\"areaWidth\":200}"}' --allow-all-permissions $ROOT

# Экспорт для движка (PluginMeshResult)
& $ENGINE --bus-call foliage_un06.export_engine_payload --payload '{"storeKey":"my_scene"}' --allow-all-permissions $ROOT
```

---

## Типичный рабочий процесс — от начала до конца

### 1. Создать лес с травой, кустами и деревьями
```powershell
$ENGINE = "C:\mont-ed\build-codex-new\Debug\EditorApp.exe"
$ROOT = "--plugin-root C:\\mont-ed-plg"

# Multi-type scatter: трава 60%, кусты 25%, деревья 15%
& $ENGINE --bus-call foliage_un06.scatter_multi_type --payload '{"storeKey":"my_forest","types":[{"name":"grass_default","weight":0.60},{"name":"bush_default","weight":0.25},{"name":"tree_pine","weight":0.15}],"areaWidth":500,"areaDepth":500,"seed":42,"includePreviewMesh":true}' --allow-all-permissions $ROOT
```

### 2. Посмотреть что получилось
```powershell
& $ENGINE --bus-call foliage_un06.analyze --payload '{"storeKey":"my_forest","areaWidth":500,"areaDepth":500}' --allow-all-permissions $ROOT
& $ENGINE --bus-call foliage_un06.compute_bounds --payload '{"storeKey":"my_forest"}' --allow-all-permissions $ROOT
& $ENGINE --bus-call foliage_un06.coverage_analysis --payload '{"storeKey":"my_forest","cellSize":20}' --allow-all-permissions $ROOT
```

### 3. Улучшить — добавить естественности
```powershell
# Шум для вариации плотности
& $ENGINE --bus-call foliage_un06.noise_modulate --payload '{"storeKey":"my_forest","noiseScale":50,"noiseThreshold":0.35,"noiseOctaves":4}' --allow-all-permissions $ROOT

# Jitter позиций
& $ENGINE --bus-call foliage_un06.jitter_positions --payload '{"storeKey":"my_forest","maxJitter":2,"seed":99}' --allow-all-permissions $ROOT

# Привязать к terrain
& $ENGINE --bus-call foliage_un06.snap_to_terrain --payload '{"storeKey":"my_forest"}' --allow-all-permissions $ROOT
```

### 4. Настроить LOD и culling
```powershell
& $ENGINE --bus-call foliage_un06.auto_lod --payload '{"importanceClass":"tree","storeKey":"my_forest"}' --allow-all-permissions $ROOT
& $ENGINE --bus-call foliage_un06.cull_distance --payload '{"action":"set","type":"tree_pine","startCullDistance":200,"endCullDistance":25000}' --allow-all-permissions $ROOT
```

### 5. Сохранить и экспортировать
```powershell
# Сохранить состояние
& $ENGINE --bus-call foliage_un06.export_state --payload '{"storeKey":"my_forest","path":"build/my_forest.json"}' --allow-all-permissions $ROOT

# Экспорт CSV для Blender/Houdini
& $ENGINE --bus-call foliage_un06.export_instances_csv --payload '{"storeKey":"my_forest","path":"build/my_forest.csv"}' --allow-all-permissions $ROOT

# Экспорт для движка (3D viewport)
& $ENGINE --bus-call foliage_un06.export_engine_payload --payload '{"storeKey":"my_forest"}' --allow-all-permissions $ROOT
```

---

## Таблица типов растительности по умолчанию

| Тип | density | scaleMin/Max | slopeMax | seed | mesh |
|-----|---------|-------------|----------|------|------|
| **grass_default** | 0.15 | 0.5 — 1.2 | 45° | 1337 | cross_billboard |
| **bush_default** | 0.08 | 0.4 — 1.0 | 50° | 777 | cube |
| **tree_pine** | 0.005 | 0.9 — 1.6 | 35° | 42 | cube |

---

## Таблица LOD рекомендаций (auto_lod)

| Класс | LOD0 | LOD1 | LOD2 | Billboard | Start Cull | End Cull |
|-------|------|------|------|-----------|-----------|----------|
| grass | 150 | 600 | 1,500 | 2,500 | 1 | 3,000 |
| bush | 300 | 1,200 | 2,500 | 5,000 | 1 | 8,000 |
| tree | 600 | 2,500 | 5,000 | 12,000 | 100 | 25,000 |
| rock | 400 | 2,000 | 4,000 | 8,000 | 50 | 15,000 |
| decor | 250 | 1,000 | 2,500 | 5,000 | 10 | 10,000 |

---

## Где смотреть

| Что | Где |
|-----|-----|
| Исходный код | `C:\mont-ed-plg\foliage_un06\src\` |
| Документация | `C:\mont-ed-plg\foliage_un06\docs\` |
| Demo payloads | `C:\mont-ed-plg\foliage_un06\demo\payloads\` |
| JSON схемы | `C:\mont-ed-plg\foliage_un06\schemas\` |
| Сборка | `.\build.ps1` |
| Тесты | `.\test.ps1`, `.\verify_build.ps1` |
| Golden tests | `.\build\golden_test.exe` |
| GitHub | https://github.com/wieiner/foliage_un06 |
