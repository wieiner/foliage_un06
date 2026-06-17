# Как пользоваться workspace

Этот workspace — рабочая папка конкретного плагина Mont-ED. Соседняя папка `foliage_un06_prompts` содержит промпты; код и локальные отчёты меняются здесь.

## Быстрый старт

```powershell
pwsh .\build.ps1
pwsh .\test.ps1
```

Если обе команды прошли, откройте:

```text
..\foliage_un06_prompts\00_START_HERE\01_PROMPT_SIDECAR_MAP.md
```

## Что дать агенту

Для первой итерации дайте агенту:

1. `AGENTS.md`
2. `TASK.md`
3. `PLUGIN_SPEC.md`
4. `PROTOCOLS.md`
5. один prompt из `..\foliage_un06_prompts\01_COPY_PASTE_FIRST`
6. один phase или task packet

Слабому агенту не давайте всю библиотеку сразу. Сильному агенту можно дать review prompt, но сначала потребуйте findings, а не автоматическую перепись.

## Как понять, что агент не ушёл в сторону

Проверьте:

- выбранный `ProtocolPreset` не изменился без причины;
- агент не добавил unselected protocol;
- `plugin.monted.json` согласован с `PROTOCOLS.md`;
- build/test реально запускались;
- `docs/phase_report.md` создан или обновлён;
- следующий шаг маленький и конкретный.

## Customizer

Если идея плагина специфическая, используйте sidecar:

```text
..\foliage_un06_prompts\07_CUSTOMIZER\CUSTOMIZATION_INPUT.md
```

Опишите конкретную цель плагина, non-goals, протоколы, ограничения и первый milestone. Затем запустите генерацию custom prompts из той же папки.
