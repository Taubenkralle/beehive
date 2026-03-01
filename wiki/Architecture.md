# Architecture Overview

---

## Code Structure

```
src/
  main.c                    ← Entry point, game loop, input handling
  kern/                     ← Core data types (headers only, no logic)
    spielzustand.h          ← Master game state struct + window constants
    zelle.h                 ← ZellenTyp enum + Zelle struct
    biene.h                 ← BienZustand, BienRolle, BienBewegung, Biene struct
    auftrag.h               ← AuftragsTyp enum + Auftrag struct
    trachtquelle.h          ← TrachtTyp enum + Trachtquelle struct
  szenen/                   ← View renderers (read simulation state, draw it)
    bienenstock.h/.c        ← ANSICHT_STOCK: hive cross-section
    wiese.h/.c              ← ANSICHT_WIESE: meadow top-down
  sim/                      ← Simulation modules (all logic lives here)
    pheromon.h/.c           ← Pheromone grids: diffusion + decay
    haushalt.h/.c           ← Pure physics formulas (no state)
    agent_biene.h/.c        ← Per-bee AI: Sense → Decide → Act → Pay
    sim_stock.h/.c          ← Colony-level simulation
    sim_welt.h/.c           ← Outside world: weather, forage sources
    pfad_stock.h/.c         ← Indoor flowfields (5 zones)
    pfad_welt.h/.c          ← Outdoor flowfields (per source + home)
memory/                     ← Claude session memory (not game data)
assets/                     ← Textures, sounds, fonts (empty for now)
wiki/                       ← This documentation
Makefile
CLAUDE.md
```

---

## The Master State (`Spielzustand`)

Everything lives in one `static Spielzustand spiel` in `main.c`. No dynamic allocation. All subsystems read and write this struct.

```c
typedef struct {
    // View
    Ansicht ansicht;           // ANSICHT_STOCK or ANSICHT_WIESE
    float   zeit;              // Elapsed time in seconds

    // Resources (real physical units)
    float honig_reif_g;        // Ripe honey (grams)
    float nektar_unreif_g;     // Nectar being processed
    float pollen_g;            // Pollen reserve
    float wasser_g;            // Water reserve
    float wachs_mg;            // Wax reserve (milligrams)

    // Brood counts per stage
    int brut_ei, brut_larve, brut_puppe;

    // Climate
    float temperatur_c;        // Brood nest temperature (target 35 °C)
    float bedrohungslevel;     // Alarm pheromone level at entrance

    // Weather
    float tageszeit;           // 0.0 = midnight, 0.5 = noon, 1.0 = midnight
    float aussentemp_c;        // Outside temperature
    float wind_staerke;        // 0.0 (calm) to 1.0 (storm)
    bool  regen;               // Rain blocks all flight
    float wetter_timer;
    bool  flug_gesperrt;       // Computed from weather; read by agent_biene

    // Flat simulation arrays
    Biene        bienen[1000];
    Zelle        zellen[578];
    Auftrag      auftraege[256];
    Trachtquelle quellen[64];
    int bienen_anzahl, zellen_anzahl, auftraege_anzahl, quellen_anzahl;

    // Pheromone fields
    PheromonFeld pheromonfelder[3];
    bool         pheromon_debug;

    // Legacy HUD fields
    int   volksgroesse;
    float honig, pollen;       // 0.0–1.0 normalised, kept in sync
} Spielzustand;
```

**Why static?** The struct is ~300+ KB. Stack allocation would overflow. Heap allocation would require a pointer everywhere. `static` gives a fixed address, zero-initialised, no fragmentation.

---

## Game Loop Order

```
Startup:
  bienenstock_init   → fill 578 hex cells in spiel->zellen[]
  wiese_init         → place decorative flowers, trees, meadow bees
  pheromon_init      → zero all three 128×72 grids
  haushalt_selbsttest → verify physics formulas with assertions
  welt_init          → place 4 forage sources, set start time
  stock_init         → set resources, count brood, spawn 10 starting bees
  pfad_stock_init    → BFS-compute 5 indoor flowfields
  pfad_welt_init     → BFS-compute outdoor flowfields per source

Per Frame (60 FPS target):
  1. Input handling (TAB, P, LMB, S)
  2. welt_aktualisieren        → advance time, update weather, refill sources
  3. pheromon_aktualisieren    → diffusion + decay step
  4. alle_bienen_aktualisieren → run AI for all active bees
  5. stock_aktualisieren       → brood, thermoregulation, nectar, job queue
  6. [scene update]            → bienenstock_aktualisieren or wiese_aktualisieren
  7. BeginDrawing()
  8. [scene render]            → bienenstock_zeichnen or wiese_zeichnen
  9. [pheromone overlay]       → if pheromon_debug
  10. EndDrawing()
```

**Key rule:** Renderers only read `spiel` — they never write it. The simulation runs at full speed regardless of which view is visible.

---

## Data Design Philosophy

### Flat Arrays (no malloc per bee)

```c
Biene bienen[1000];   // NOT: Biene** bienen; bienen[i] = malloc(sizeof(Biene))
```

All bees live in a contiguous memory block. This is cache-friendly: iterating over all bees reads memory linearly, which modern CPUs prefetch efficiently.

Dead bees have `aktiv = false`. Their slot is reused by the next hatching bee. No freeing, no fragmentation, no pointer chasing.

### One File = One Responsibility

Each `.c` file owns exactly one concern:
- `sim_stock.c` — colony-level simulation
- `agent_biene.c` — per-bee AI
- `bienenstock.c` — rendering the hive view
- Never mix simulation logic into renderers, or rendering into simulation

### Headers First

Every `.h` file defines the full public API before the `.c` file is written. This forces clear interface design and makes the whole project understandable from headers alone.

---

## Hex Grid

The hive view uses a **flat-top hexagon** grid:

```
Radius (tip to tip / 2):  18 px
Column step (x):          18 × 1.5   = 27 px
Row step (y):             18 × √3    ≈ 31.2 px
Odd column offset (y):    18 × 0.866 ≈ 15.6 px
Grid start:               (162, 72)
Grid size:                34 columns × 17 rows = 578 cells
```

Cell index formula: `index = zeile × HEX_SPALTEN + spalte`

Cell type is determined by distance from the centre (`berechne_zellentyp`):

| Distance from centre | Zone |
|---------------------|------|
| < 18% of radius | Brood eggs (inner core) |
| 18–50% | Brood larvae + some pollen |
| 50–80% | Honey + pollen |
| > 80% | Honey + empty (outer ring) |

With deterministic noise added per cell for natural variation.

---

## Build

```bash
make        # compile only → ./beehive
make run    # compile + launch
make clean  # remove .o files and binary
```

Compiler: `gcc -Wall -O2`
Dependencies: raylib 5.5 (via Homebrew at `/opt/homebrew`)
Platform: macOS (Apple Silicon M2, OpenGL 4.1 Metal)
