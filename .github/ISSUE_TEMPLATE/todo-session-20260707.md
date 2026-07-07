---
name: "Session TODO 2026-07-07"
about: "Remaining tasks after 2026-07-07 session"
title: "Session 2026-07-07 remaining TODOs"
labels: enhancement
assignees: dmsovenko-ship-it
---


## High Priority

1. **Incursion scanner visibility** — After `MakeDungeon` + `AddSignal`, set `AttrSignatureRadius` on root item so anomalies appear with proper strength (WIP in `SpawnSites`)
2. **`config::GetMapConnections`** — Auto-generated cache method expects `PyInt` but client sends `PyBool` for params 2-4 (`fromMe`, `fromCorp`, `fromAlliance`). Need to fix cache definition.
3. **Ship Fittings Manager** — Full CRUD: create `chrFittings` SQL table + implement `CharFittingMgr`/`CorpFittingMgr` methods (`SaveFitting`, `GetFittings`, `DeleteFitting`, `UpdateNameAndDescription`)

## Medium Priority

4. **`OnSlimItemChange` destiny format** — Client receives event with int ID instead of string `"OnSlimItemChange"`. Possible destiny update encoding issue.
5. **`SpaceView.keyDownEvent`** — Client-side crash when closing map/info window. Triggered by preceding failures.

## Low Priority

6. **Remove debug logging** from `SpawnMgr.cpp` (`DoSpawnForAnomaly` DEBUG lines)
7. **Fresh DB rebuild** — `docker-compose down -v && docker-compose up --build -d` to verify all migrations
8. **Various header stub cleanup** — `PassiveModule()`, `RigModule()`, `SubSystemModule()` empty constructors
