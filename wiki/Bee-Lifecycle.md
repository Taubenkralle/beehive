# Bee Lifecycle

Every bee in the simulation passes through four development stages before becoming a worker.

---

## Development Stages

```
EGG  →  LARVA  →  PUPA  →  ADULT
 3d       6d       12d      21–45d lifespan
```

All timers use **simulated days** (`alter_t`). One real second = 1/86400th of a simulated day at simulation speed. One simulated day = 120 real seconds (`SIM_TAG_DAUER_S = 120`), so a full egg-to-adult cycle takes about **42 minutes** of real play time.

---

## Egg (`ZELLE_BRUT_EI`)

- Laid by the queen in a clean, empty cell
- Duration: **3 simulated days** (`EI_DAUER_T = 3.0`)
- No feeding required at this stage
- If the cell temperature drops far below 35 °C, mortality risk increases
- After 3 days → cell type changes to `ZELLE_BRUT_LARVE`

## Larva (`ZELLE_BRUT_LARVE`)

- Duration: **6 simulated days** (`LARVEN_DAUER_T = 6.0`)
- Requires active feeding by nurse bees (`AMME` role)
  - Each nurse consumes honey + pollen from colony reserves
- **Starvation risk**: if pollen reserves < 5 g, mortality probability spikes
- **Thermal stress**: deviation from 35 °C raises `p_tod` (death probability)
- Mortality formula: `brut_sterblichkeit(temperatur_c, nahrungs_mangel)`
- After 6 days → cell type changes to `ZELLE_BRUT_PUPPE`

## Pupa (`ZELLE_BRUT_PUPPE`)

- Duration: **12 simulated days** (`PUPPEN_DAUER_T = 12.0`)
- No feeding, but still temperature-sensitive (less tolerant than larvae)
- Thermal stress can still kill at this stage
- After 12 days → pupa hatches: `biene_schluepfen()` is called
  - A new adult bee (role: `PUTZERIN`) spawns at the colony storage zone
  - The cell becomes `ZELLE_LEER` with `sauber = false` (needs cleaning)

## Adult

New adults start as `PUTZERIN` (cell cleaner) — the youngest role. Age-based role progression happens automatically:

| Age (days) | Role assigned |
|------------|--------------|
| < 3 | `PUTZERIN` — cleans cells after hatching |
| < 10 | `AMME` — feeds larvae |
| < 14 | `BAUERIN` — builds comb |
| < 21 | `EMPFAENGERIN` / `FAECHER` — receives nectar, fans air |
| ≥ 24 | `SAMMLERIN_*` — forages outside the hive |

See [Roles & Tasks](Roles-and-Tasks) for full details.

---

## Initial Brood Ages

At startup, `bienenstock_init()` fills all 578 hex grid cells using `berechne_zellentyp()`. Brood cells are given **randomised starting ages** to prevent mass-synchronised hatching:

| Stage | Random age range |
|-------|-----------------|
| Egg | 0.0 – 3.0 days |
| Larva | 0.0 – 6.0 days |
| Pupa | 0.0 – 12.0 days |

---

## Death & Slot Reuse

Bees that die (energy = 0, or old age) have `aktiv = false` set. Their slot in the flat `bienen[]` array is reused by the next hatching bee — no memory allocation needed.

---

## Mortality Formula

```c
float brut_sterblichkeit(float temperatur_c, float nahrungs_mangel);
```

- Base mortality: very small per tick
- Temperature stress: increases sharply outside 32–36 °C range
- Starvation factor: added on top when `nahrungs_mangel > 0`
- Applied probabilistically per tick using a deterministic hash of `alter_t`
