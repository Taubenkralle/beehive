# Outside World & Weather

The meadow view simulates the environment surrounding the hive — forage sources, time of day, temperature, wind, and rain.

---

## Time of Day

```c
#define SIM_TAG_DAUER_S  120.0f   // one simulated day = 120 real seconds
```

`tageszeit` runs from `0.0` (midnight) to `1.0` (next midnight):
- `0.25` = 06:00 (dawn)
- `0.50` = 12:00 (noon)
- `0.75` = 18:00 (dusk)

The HUD in Meadow View shows current time as `hh:mm`.

### Effect on Foraging

Bees cannot fly outside the window `[0.10, 0.90]` (roughly 02:30–21:36). Outside those hours, `flug_gesperrt = true`.

Forage source yield follows a sine curve peaking at noon:
```c
tageszeit_faktor = sinf(tageszeit × π)   // 0 at dawn/dusk, 1 at noon
```

---

## Weather

`wetter_aktualisieren()` runs every tick and updates:

### Wind

Wind speed (`wind_staerke`, 0.0–1.0) uses overlapping sine waves for natural variation:

```c
wind_staerke = 0.5 + 0.4 × sin(zeit × 0.017) + 0.1 × sin(zeit × 0.053)
```

No random number involved — deterministic, but looks organic. Wind above **0.8** blocks all flight.

### Rain

Rain uses a simple state machine with `rand()`:

```
CHECKING: every WETTER_INTERVALL_S = 20 seconds
  → 20% chance: start raining  (rain_duration = 15–45 sim-seconds)
  → while raining: 10% chance per check: stop rain early
```

Rain immediately sets `flug_gesperrt = true`. All foragers that are currently outside cannot return immediately — they shelter until conditions improve (not yet implemented; currently they finish their current task then stop requesting new outdoor jobs).

### Temperature

Outside temperature follows a day/night cycle:

```c
#define AUSSENTEMP_BASIS_C       18.0f   // average temperature
#define AUSSENTEMP_AMPLITUDE      8.0f   // swing above/below

aussentemp_c = AUSSENTEMP_BASIS_C + AUSSENTEMP_AMPLITUDE × sin(tageszeit × π)
```

Range: 10 °C at night → 26 °C at noon. Below **10 °C**, flight is blocked (`FLUG_TEMP_MIN_C`).

---

## Flight Block (`flug_gesperrt`)

`flug_moeglich()` returns `false` (and sets `flug_gesperrt = true`) when **any** of these apply:

| Condition | Threshold |
|-----------|-----------|
| Rain | `regen == true` |
| Cold | `aussentemp_c < 10 °C` |
| Storm | `wind_staerke > 0.8` |
| Night | `tageszeit < 0.10 || tageszeit > 0.90` |

When flight is blocked, `agent_biene.c` skips all outdoor job assignments.

---

## Forage Sources (`Trachtquelle`)

Four sources are placed at startup by `welt_init()`:

| Type | World Position | Capacity |
|------|---------------|---------|
| Nectar (`TRACHT_NEKTAR`) | (750, 220) | 400 g |
| Pollen (`TRACHT_POLLEN`) | (950, 380) | 200 g |
| Water (`TRACHT_WASSER`) | (680, 560) | 1000 g |
| Nectar 2 (`TRACHT_NEKTAR`) | (1050, 180) | 250 g |

### Source Data

```c
typedef struct {
    TrachtTyp typ;
    float pos_x, pos_y;          // World position
    float vorrat_g;              // Current supply (grams)
    float kapazitaet_g;          // Maximum capacity
    float regeneration_g_s;      // Refill rate per second
    float risiko;                // Danger level (0.0 = safe, 1.0 = deadly)
    bool  aktiv;                 // Accessible to bees
} Trachtquelle;
```

### Regeneration

Each source refills passively each tick, modified by conditions:

```
yield_factor = tageszeit_faktor × temp_factor × wind_factor × rain_factor
vorrat_g += regeneration_g_s × yield_factor × dt
vorrat_g = min(vorrat_g, kapazitaet_g)
```

- `rain_factor = 0` when raining (flowers closed)
- `wind_factor = 1 − wind_staerke × 0.5` (wind reduces nectar flow)
- `temp_factor` increases with warmth

---

## Meadow View Indicators

In the Meadow View, forage sources are drawn as circles:
- **Outer ring** — source outline (colour by type: orange=nectar, yellow=pollen, blue=water)
- **Filled disc** — current fill level (proportional to `vorrat_g / kapazitaet_g`)
- **Letter label** — N (Nectar), P (Pollen), W (Water)

When a source depletes, the filled disc shrinks to nothing while the ring remains visible.

---

## Hive Entrance (Portal)

The hive entrance acts as the **portal** between the two views:

- Indoor position: `STOCK_EINGANG_X = 130, STOCK_EINGANG_Y = 370` (inside the hive, left side)
- Outdoor position: `STOCK_EINGANG_X = 130, STOCK_EINGANG_Y = 370` (same coords, stock building on left of meadow)

Bees transition: `ZUM_AUSGANG` → cross the threshold → `AUSSENFLUG`. On return: `HEIMFLUG` → arrive → `ZUM_ABLADEN`.
