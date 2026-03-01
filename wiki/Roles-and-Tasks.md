# Roles & Tasks

The colony runs on a **demand-driven job queue**. Every tick, `stock_auftraege_generieren()` measures what the colony needs, posts jobs to the queue, and bees pick up whichever job suits their role and age.

---

## Worker Roles (`BienRolle`)

| Code | Role | Description |
|------|------|-------------|
| `PUTZERIN` | Cell cleaner | Cleans empty cells so the queen can lay eggs |
| `AMME` | Nurse | Feeds larvae (consumes honey + pollen) |
| `BAUERIN` | Builder | Converts wax into new comb cells |
| `EMPFAENGERIN` | Receiver | Takes incoming nectar from foragers and processes it |
| `FAECHER` | Fanner | Fans air to evaporate water from nectar; cools the hive |
| `WAECHTER` | Guard | Stays at the entrance; holds off intruders |
| `BESTATERIN` | Undertaker | Removes dead bees from the hive |
| `SAMMLERIN_NEKTAR` | Nectar forager | Flies out and collects nectar from flowers |
| `SAMMLERIN_POLLEN` | Pollen forager | Collects pollen (protein source for larvae) |
| `SAMMLERIN_WASSER` | Water forager | Fetches water for cooling and diluting honey |
| `SAMMLERIN_PROPOLIS` | Propolis forager | Collects resin for sealing gaps and hygiene |
| `ERKUNDERIN` | Scout | Explores for new forage patches |

### Age-Based Progression

Real honey bees change roles as they age (temporal polyethism). The simulation mirrors this:

```
New hatching  →  PUTZERIN  →  AMME  →  BAUERIN  →  FAECHER/EMPFAENGERIN  →  SAMMLERIN
    day 0           day 3       day 10     day 14           day 21                day 24+
```

`biene_rolle_waehlen()` runs every tick and compares `b->alter_t` against thresholds. If colony needs override (e.g. desperate for nurses), a bee can be kept in a role longer.

---

## Movement States (`BienBewegung`)

Each bee has a state machine with 7 states:

| Code | State | Description |
|------|-------|-------------|
| `IM_STOCK_ARBEITEND` | Working inside | Executing an indoor job at target location |
| `ZUM_AUSGANG` | Going to exit | Walking toward the hive entrance |
| `AUSSENFLUG` | Outside travel | Flying to a forage source |
| `AM_SAMMELN` | Foraging | Actively collecting resource at source |
| `HEIMFLUG` | Returning | Flying back to the hive |
| `ZUM_ABLADEN` | Searching drop-off | Looking for storage location inside |
| `RASTEN` | Resting | Recovering energy |

State transitions are driven by `biene_aktualisieren()` → `biene_entscheiden()`.

---

## The Job Queue

Jobs live in `spiel->auftraege[]` (max 256 slots). Each `Auftrag` has:

```c
typedef struct {
    AuftragsTyp typ;
    int   ziel_zelle;     // target cell index (-1 = zone-based)
    float ziel_x, ziel_y; // target world position
    int   prioritaet;     // 1 = highest
    bool  vergeben;       // already claimed by a bee
    bool  aktiv;          // slot is in use (false = reusable)
} Auftrag;
```

### Index Stability

When a bee claims a job, it stores the **array index** (`auftrag_id`). Compacting the array by shifting would corrupt these references. Instead, completed/cancelled jobs have `aktiv = false` and their slots are **reused in-place** by new jobs. No shifting, no broken references.

### Job Types

**Indoor jobs:**

| Code | Priority | Triggered when |
|------|----------|---------------|
| `AUFTRAG_LARVE_FUETTERN` | 1 | `brut_larve > 0` |
| `AUFTRAG_EINGANG_BEWACHEN` | 1 | Always at least 1 guard |
| `AUFTRAG_FAECHELN` | 2 | `temperatur_c > 36 °C` |
| `AUFTRAG_NEKTAR_VERARBEITEN` | 2 | `nektar_unreif_g > 0.1` |
| `AUFTRAG_ZELLE_REINIGEN` | 2 | Dirty empty cells exist |
| `AUFTRAG_WABE_BAUEN` | 3 | `wachs_mg ≥ 1300 mg` |

**Outdoor jobs:**

| Code | Priority | Triggered when |
|------|----------|---------------|
| `AUFTRAG_NEKTAR_SAMMELN` | 3 | Always (≈30% of colony) |
| `AUFTRAG_POLLEN_SAMMELN` | 3 | `pollen_g < 100 g` |
| `AUFTRAG_WASSER_HOLEN` | 3 | On demand |
| `AUFTRAG_PROPOLIS_HOLEN` | 3 | On demand |

Outdoor jobs are **blocked when `flug_gesperrt = true`** (rain, storm, night — see [Outside World](Outside-World)).

---

## How a Bee Picks a Job

1. Bee enters `RASTEN` or finishes previous job → `biene_auftrag_holen()` called
2. Scans `auftraege[]` for: `aktiv && !vergeben && typ matches rolle`
3. Picks lowest `prioritaet` number (highest urgency)
4. Sets `vergeben = true`, stores index as `b->auftrag_id`
5. Sets movement state: indoor → `IM_STOCK_ARBEITEND`, outdoor → `ZUM_AUSGANG`

When the job is done, `auftrag_freigeben()` sets `aktiv = false`.

---

## Carrying Loads

Forager bees carry resources in dedicated fields on the `Biene` struct:

| Field | Contents |
|-------|----------|
| `honigmagen_mg` | Nectar in the honey stomach (mg) |
| `pollenladung_mg` | Pollen on hind legs (mg) |
| `wasser_mg` | Water carried (mg) |
