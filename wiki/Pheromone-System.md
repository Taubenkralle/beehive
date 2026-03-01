# Pheromone System

Bees communicate almost entirely through chemicals. The simulation models three pheromone types as independent 2D concentration grids covering the full window.

---

## Pheromone Types

| Code | Type | Real-world role |
|------|------|----------------|
| `PHEROMON_ALARM` | Alarm pheromone | Released when hive is threatened; recruits guards, triggers defensive behaviour |
| `PHEROMON_SPUR` | Trail pheromone | Marks the route from hive to a good forage source; recruits other foragers |
| `PHEROMON_NASONOV` | Nasonov pheromone | Orientation signal — "the hive entrance is here"; helps returning bees find home |

---

## The Grid

Each pheromone type has its own `PheromonFeld` struct:

```c
#define PHEROMON_BREITE 128   // horizontal cells
#define PHEROMON_HOEHE   72   // vertical cells
// Total: 9216 float values per field, covering the full 1280×720 window
// Each cell = 10×10 pixels

typedef struct {
    float zellen[PHEROMON_BREITE * PHEROMON_HOEHE];
    float diffusion;   // D — how fast it spreads outward
    float zerfall;     // k — how fast it fades
} PheromonFeld;
```

---

## Physics

Every tick, `pheromon_aktualisieren()` applies the diffusion-decay equation to all three fields:

```
C(x,y,t+dt) = C(x,y,t)
             + D × Laplace(C) × dt    ← diffusion (spreads outward)
             − k × C × dt            ← decay (fades over time)
             + S(x,y) × dt           ← source term (bee emitting)
```

`Laplace(C)` is the discrete 4-neighbour approximation:
```
Laplace = C[x-1,y] + C[x+1,y] + C[x,y-1] + C[x,y+1] − 4×C[x,y]
```

### Tuning Constants

| Pheromone | Diffusion (D) | Decay (k) | Character |
|-----------|--------------|-----------|-----------|
| Alarm | Fast | Fast | Urgent, short-lived signal |
| Trail | Medium | Slow | Persistent route marker |
| Nasonov | Slow | Medium | Steady beacon near home |

---

## Emission

```c
void pheromon_abgeben(PheromonFeld* feld, float x, float y, float menge);
```

Adds `menge` to the grid cell at world position `(x, y)`. Bees emit pheromones contextually:

- **Forager returning with nectar** → emits trail pheromone along the route
- **Guard detecting intruder** → emits alarm pheromone at entrance
- **Bee near the entrance** → emits Nasonov pheromone (helps swarm orientation)

---

## Reading the Field

```c
float pheromon_lesen(const PheromonFeld* feld, float x, float y);
```

Uses **bilinear interpolation** between the four surrounding grid cells, so bees get a smooth gradient value rather than a stepped one. This is important for gradient-following behaviour.

---

## Debug Overlay

Press **P** in-game to toggle the pheromone heatmap overlay:

- **Red/Orange** — Alarm pheromone
- **Green/Yellow** — Trail pheromone
- **Blue** — Nasonov pheromone

The overlay renders on top of the scene, underneath the HUD.

Debug controls:
- **LMB** — Manually place alarm pheromone at mouse position
- **S (held)** — Manually place trail pheromone at mouse position

---

## Biological Notes

Real honey bees have over 15 known pheromone types. The simulation currently models the three most critical for colony behaviour:

- **Alarm (isoamyl acetate):** Released from the sting gland. Triggers mass defensive response. Volatile — disappears in minutes.
- **Trail / Footprint pheromone:** Deposited on surfaces. Marks well-used paths.
- **Nasonov (geraniol + other terpenoids):** Fanning bee exposes gland near the abdomen tip, releasing the scent upward. Critical during swarming for orientation.
