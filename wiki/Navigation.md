# Navigation & Pathfinding

With potentially thousands of bees moving simultaneously, traditional pathfinding (A* per bee) would be far too expensive. Instead, the simulation uses **flowfields** — a technique where pathfinding is computed once per destination, then any number of bees can read their direction in O(1) per frame.

---

## The Core Idea

A flowfield is a 2D grid where every cell stores a direction vector pointing toward the goal. Instead of each bee solving its own path, every bee at position `(x, y)` simply looks up the pre-computed direction:

```
bee wants to go to the honey storage zone
→ look up STOCK_ZONE_HONIG flowfield at cell (bee.pos_x/20, bee.pos_y/20)
→ get direction vector (dx, dy)
→ move that way
```

10,000 bees = 10,000 grid lookups. No search, no priority queues.

---

## Grid Specification

```c
#define PFAD_ZELLEN_GROESSE   20    // pixels per cell
#define PFAD_FF_BREITE        64    // 1280 / 20
#define PFAD_FF_HOEHE         36    // 720 / 20
#define PFAD_FF_ZELLEN       2304   // 64 × 36
```

Each cell is a `Richtung` (direction) struct:
```c
typedef struct { int8_t dx, dy; } Richtung;   // values: -1, 0, or +1
```

Diagonal directions are normalised by dividing by √2 ≈ 0.707 when converting to world movement.

---

## BFS Computation

The flowfield for each goal is computed using **Breadth-First Search starting from the goal** (backwards BFS):

```
1. Mark goal cell as cost 0, push to queue
2. For each cell popped:
   a. For each of 8 neighbours (including diagonal):
      - If not visited: cost = parent_cost + 1, push to queue
      - Set direction: neighbour → parent (toward goal)
3. All cells now have a direction pointing toward the goal
```

This is done once at startup. Obstacles can be added later (Phase 9: walls, wax structures) as impassable cells in the BFS.

---

## Indoor Flowfields (5 Zones)

`pfad_stock_init()` computes one flowfield per zone:

| Zone | Code | Goal position | Used by |
|------|------|-------------|---------|
| Entrance | `STOCK_ZONE_EINGANG` | (180, 370) | Bees going outside (`ZUM_AUSGANG`) |
| Storage | `STOCK_ZONE_LAGER` | (640, 360) | Bees unloading (`ZUM_ABLADEN`) |
| Brood | `STOCK_ZONE_BRUT` | (400, 360) | Nurses, cleaners |
| Honey | `STOCK_ZONE_HONIG` | (680, 250) | Processors, builders |
| Pollen | `STOCK_ZONE_POLLEN` | (680, 470) | Pollen storage work |

```c
Vector2 pfad_stock_richtung(float pos_x, float pos_y, StockZone zone);
```

---

## Outdoor Flowfields (per Source + Home)

`pfad_welt_init()` computes one flowfield per active forage source, plus one field pointing toward the hive entrance:

```c
Vector2 pfad_welt_richtung(float pos_x, float pos_y, int quellen_id);  // → forage source
Vector2 pfad_eingang_richtung(float pos_x, float pos_y);              // → hive entrance
```

Foragers use `quellen_id_fuer_biene()` to look up which source their current job targets.

Flowfields are cached in static arrays and marked valid/invalid. If a source moves or is added, `pfad_welt_berechnen(id)` recomputes just that one field.

---

## How `biene_bewegen()` Uses Flowfields

```c
void biene_bewegen(Biene* b, const Spielzustand* spiel, float dt);
```

State → flowfield mapping:

| Bee state | Flowfield used |
|-----------|---------------|
| `ZUM_AUSGANG` | `pfad_stock_richtung(STOCK_ZONE_EINGANG)` |
| `ZUM_ABLADEN` | `pfad_stock_richtung(STOCK_ZONE_LAGER)` |
| `IM_STOCK_ARBEITEND` | `pfad_stock_richtung(zone_fuer_rolle(rolle))` |
| `AUSSENFLUG` | `pfad_welt_richtung(quellen_id)` |
| `HEIMFLUG` | `pfad_eingang_richtung()` |
| `RASTEN`, `AM_SAMMELN` | No movement |

**Fallback:** If the flowfield returns a zero vector (goal cell or uninitialised), the bee falls back to **direct steering** — computing the direction to `(ziel_x, ziel_y)` manually.

**Overshoot prevention:** If the bee is within `geschwindigkeit × dt` pixels of its target, it snaps directly to the target instead of overshooting.

---

## Performance

| Approach | Cost per bee per frame |
|----------|----------------------|
| A* per bee | O(N log N) grid cells searched |
| Flowfield lookup | O(1) — single array read |

At 60 FPS with 1,000 bees: **60,000 O(1) lookups** vs 60,000 pathfinding searches. The difference becomes dramatic at 10,000+ bees — which is the design target.

---

## Future: Obstacle Maps

Currently the BFS has no obstacles — every cell is passable. The flowfield infrastructure is ready for Phase 9 to add:

- Wax comb walls as impassable cells
- Crowding penalties (cell cost increases when many bees are nearby)
- Danger zones (repulsion fields near threats)

Adding obstacles simply means marking cells as `cost = UINT16_MAX` before running BFS.
