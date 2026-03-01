# Resources

The colony tracks five physical resources, all stored in **real units** (grams or milligrams) rather than abstract percentages.

---

## Resource Fields

| Field | Unit | Description |
|-------|------|-------------|
| `honig_reif_g` | grams | Ripe honey — long-term energy store |
| `nektar_unreif_g` | grams | Unripe nectar being processed |
| `pollen_g` | grams | Pollen — protein source for larvae |
| `wasser_g` | grams | Water — cooling and nectar dilution |
| `wachs_mg` | milligrams | Wax — used to build new comb cells |

---

## Nectar → Honey Pipeline

This is the most complex resource chain. It takes several real-world steps to turn raw flower nectar into shelf-stable honey.

```
FLOWER NECTAR (30% sugar)
       ↓  forager collects → honigmagen_mg
       ↓  forager returns → transfers to receiver bee
       ↓  added to nektar_unreif_g
       ↓  fanning bees evaporate water
       ↓  RIPE HONEY (82% sugar, 18% water)
       ↓  added to honig_reif_g
```

### Conversion Formula

```
zucker_g = nektar_g × zuckergehalt        (zuckergehalt = 0.30 for typical nectar)
honig_g  = zucker_g / (1 − wasseranteil)  (wasseranteil = 0.18 for ripe honey)
```

This means roughly **3.6 kg of nectar** yields **1 kg of honey**.

### Evaporation Rate

Fanning bees (`FAECHER` + `EMPFAENGERIN`) drive water evaporation:

```c
float nektar_verdunstung(int faecher_anzahl, float dt);
```

More fanners → faster evaporation → faster honey production.

### Conversion Rate

```c
#define FAECHER_NEKTAR_RATE_G_S  0.0005f   // grams processed per fanner per second
```

Each tick: `rate = (faecher + 1) × 0.0005 × dt` grams of concentrated nectar become ripe honey.

---

## Pollen

- Collected by `SAMMLERIN_POLLEN` from `TRACHT_POLLEN` forage sources
- Used by nurse bees (`AMME`) when feeding larvae — mixed with honey to make "bee bread"
- Critical threshold: **< 5 g** triggers larval starvation risk
- Colony generates pollen-foraging jobs when `pollen_g < 100 g`

---

## Wax Production

Wax is a byproduct of bee metabolism — bees consume honey to secrete wax flakes.

```c
#define BIENE_WACHSPROD_G_S    0.00005f  // honey consumed per builder per second
#define WACHS_PRO_ZELLE_MG    1300.0f   // wax needed to build one cell
```

```
zucker_g = honig_consumed × (1 − WASSERANTEIL_HONIG)
wachs_mg += wachs_aus_zucker(zucker_g)          // 8 mg wax per 1 g sugar
```

When accumulated wax exceeds 1300 mg, `stock_wabe_bauen()` converts it into a new `ZELLE_LEER` cell in the simulation grid.

**Biological note:** Real bees need to consume ~6–8 kg of honey to produce 1 kg of wax. The simulation uses the same ratio.

---

## Starting Resources

At colony initialisation (`stock_init`):

| Resource | Starting amount |
|----------|----------------|
| Ripe honey | 500 g |
| Unripe nectar | 0 g |
| Pollen | 80 g |
| Water | 15 g |
| Wax | 3000 mg (~2 cells worth) |

---

## Energy Economy

Every bee has a personal energy reserve (`energie_j`, in Joules):

| Activity | Power draw |
|----------|-----------|
| Resting | 0.001 W |
| Walking | 0.003 W |
| Flying | 0.014 W |

```c
energie_verbrauch(leistung_w, dt) = leistung_w × dt   // Joule = Watt × seconds
energie_aufnahme(zucker_g)        = zucker_g × 17.0    // 17 J per gram of sugar
```

When `energie_j ≤ 0`, the bee dies (`aktiv = false`). A forager that flies too far without enough energy reserves will not make it home.

---

## The Honey Stomach (`honigmagen_mg`)

Forager bees carry nectar in a specialised stomach separate from their own digestive system. Upon returning to the hive, the nectar is passed mouth-to-mouth to a receiver bee and eventually added to `nektar_unreif_g`.
