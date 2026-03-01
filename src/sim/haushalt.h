#ifndef HAUSHALT_H
#define HAUSHALT_H

// ---------------------------------------------------------------------------
// Physical constants — biologically grounded values
// ---------------------------------------------------------------------------

// Nectar → Honey
#define ZUCKERGEHALT_NEKTAR    0.30f   // 30% sugar content in average nectar
#define WASSERANTEIL_HONIG     0.18f   // 18% water in ripe honey (legal max)

// Energy
#define ENERGIE_DICHTE_J_G    17.0f   // Joules per gram of sugar (sucrose)
#define LEISTUNG_FLUG_W        0.014f  // Watts during flight (measured in bees)
#define LEISTUNG_LAUFEN_W      0.003f  // Watts while walking inside hive
#define LEISTUNG_STEHEN_W      0.001f  // Watts at rest / standing still
#define ENERGIE_START_J        1.5f    // Starting energy reserve for a new bee

// Wax production
#define WACHS_AUSBEUTE         8.0f    // mg wax produced per g sugar consumed
#define WACHS_PRO_ZELLE_MG  1300.0f   // mg wax needed to build one cell

// Nectar ripening (evaporation)
#define VERDUNSTUNG_BASIS      0.0002f // g water evaporated per second (base rate)
#define FAECHER_SKALIERUNG     0.3f    // Extra evaporation factor per fanning bee

// Brood development durations (in days, at optimal 35°C)
#define EI_DAUER_T             3.0f    // Egg stage: 3 days
#define LARVEN_DAUER_T         6.0f    // Larva stage: 6 days
#define PUPPEN_DAUER_T        12.0f    // Pupa stage: 12 days

// Recruitment (waggle dance)
#define MAX_REKRUTIERUNG      10.0f    // Max new foragers recruited per minute
#define REKRUTIERUNG_K         0.5f    // Profit scale factor for saturation curve

// ---------------------------------------------------------------------------
// API — all functions are pure (no side effects, no global state)
// ---------------------------------------------------------------------------

// Convert nectar to ripe honey (returns honey grams)
// m_honey = (nectar_g * sugar_ratio) / (1 - water_honey)
float honig_aus_nektar(float nektar_g, float zuckergehalt);

// Energy consumed by an action over time interval dt (returns Joules)
float energie_verbrauch(float leistung_w, float dt);

// Energy gained from eating sugar (returns Joules)
float energie_aufnahme(float zucker_g);

// Wax produced when a bee metabolises sugar for building (returns mg)
float wachs_aus_zucker(float zucker_g);

// Water evaporated from nectar per tick — more fanning bees = faster ripening
// Returns grams of water removed
float nektar_verdunstung(int faecher_anzahl, float dt);

// Profitability score of a forage patch (can be negative = not worth visiting)
// zucker_pro_min: sugar yield per minute at the patch
// flugzeit_s: one-way flight time in seconds
// risiko: danger level 0.0 (safe) to 1.0 (lethal)
float patch_profit(float zucker_pro_min, float flugzeit_s, float risiko);

// Number of new foragers recruited per minute via waggle dance
// Uses saturating Michaelis-Menten-style curve
int rekrutierung(float profit, float max_rekrutierung, float skalierung_k);

// Brood death probability per tick — rises with temperature stress and hunger
// Returns probability 0.0-1.0
float brut_sterblichkeit(float temperatur_c, float nahrungs_mangel);

// Self-test: asserts known-good values — call once at startup in debug builds
void haushalt_selbsttest(void);

#endif // HAUSHALT_H
