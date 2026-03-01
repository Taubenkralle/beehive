# Brood Care & Thermoregulation

Keeping the brood nest at exactly **35 °C** is one of the colony's most critical tasks. Too cold or too hot, and larvae and pupae die.

---

## Target Temperature

```
Optimal brood nest: 35 °C
Dangerous below:    32 °C
Dangerous above:    36 °C  (fanning kicks in)
Lethal extremes:    < 15 °C or > 45 °C (clamped in simulation)
```

---

## Thermoregulation Loop

Every tick, `stock_thermoregulieren()` calculates the temperature delta:

```c
float bienen_waerme   = volksgroesse × 0.003;       // metabolic heat from all bees (°C/s)
float ambient_verlust = (temperatur_c − 20.0) × 0.004; // passive cooling toward 20 °C outside
float faecheln_kuehl  = faecher_count × 0.05;        // active cooling from fanning bees

delta = (bienen_waerme − ambient_verlust − faecheln_kuehl) × dt;
temperatur_c += delta;
```

### Heating

All bees generate metabolic heat by vibrating their flight muscles. A larger colony runs warmer. In winter (not yet implemented), bees cluster tightly and vibrate continuously to survive.

### Cooling

When `temperatur_c > 36 °C`, the job queue generates `AUFTRAG_FAECHELN` tasks. Fanning bees station themselves near the entrance and beat their wings to draw cool air through the hive.

Number of fanning jobs requested:
```c
faecher_bedarf = (int)((temperatur_c − 35.0) × 4.0) + 1
```

### Fanning also dries nectar

The same fanning airflow that cools the hive also evaporates water from unripe nectar — two jobs done at once. `FAECHER` and `EMPFAENGERIN` bees both count toward the evaporation rate.

---

## Brood Mortality

`brut_sterblichkeit(temperatur_c, nahrungs_mangel)` returns a per-tick death probability.

| Condition | Effect |
|-----------|--------|
| Temperature in 32–36 °C | Minimal mortality |
| Temperature outside 32–36 °C | Rising mortality (quadratic) |
| Pollen < 5 g (`AMME` can't feed) | `nahrungs_mangel = 0.8` — high larval deaths |
| Combined stress | Additive |

Mortality is applied probabilistically using a deterministic hash of `alter_t` (not `rand()`), so results are consistent and reproducible for a given colony state.

```c
if (p_tod > 0.0f && (z->alter_t * 137.0f − (int)(z->alter_t * 137.0f)) < p_tod * tage_dt) {
    z->typ    = ZELLE_LEER;
    z->sauber = false;
}
```

---

## Nurse Bee Feeding

The `AMME` role handles larval feeding. Each nurse assigned `AUFTRAG_LARVE_FUETTERN`:
1. Navigates to the brood zone (flowfield: `STOCK_ZONE_BRUT`)
2. Spends time at the target
3. Consumes honey + pollen from colony reserves (future: per-feed amounts)
4. Marks the job done → job slot freed

The colony always posts **one feeding job per larva** in the queue:
```c
int bedarf  = spiel->brut_larve;
int bereits = offene_auftraege(spiel, AUFTRAG_LARVE_FUETTERN);
int anzahl  = bedarf − bereits;
```

---

## Comb Cells & Cleanliness

After a pupa hatches (or a larva dies), the cell becomes `ZELLE_LEER` with `sauber = false`. A dirty cell:
- Cannot be used by the queen for egg-laying
- Generates `AUFTRAG_ZELLE_REINIGEN` in the job queue
- A `PUTZERIN` claims the job, cleans it → `sauber = true`

This creates a natural bottleneck: if too many cells are dirty and not enough cleaners are available, egg-laying slows down even if space exists.

---

## Colony Feedback Loop

```
More brood → more nurses needed → more pollen consumed
     ↓                                      ↓
More foragers needed               Pollen runs low → larvae die
     ↓                                      ↓
Less brood cover → less brood → colony shrinks
```

This emergent feedback loop is the core survival mechanic. A healthy colony self-regulates; a stressed colony can spiral downward.
