#include "haushalt.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// honig_aus_nektar
//
// Biology: bees evaporate water from nectar and add enzymes.
// Sugar mass is conserved; only water changes.
//
// Step 1: extract sugar mass from nectar
//   zucker_g = nektar_g * zuckergehalt
// Step 2: calculate honey mass needed to hold that sugar at 18% water
//   honig_g  = zucker_g / (1 - WASSERANTEIL_HONIG)
// ---------------------------------------------------------------------------
float honig_aus_nektar(float nektar_g, float zuckergehalt) {
    float zucker_g = nektar_g * zuckergehalt;
    return zucker_g / (1.0f - WASSERANTEIL_HONIG);
}

// ---------------------------------------------------------------------------
// energie_verbrauch
//
// E = P * t  (Joule = Watt * Sekunden)
// Use LEISTUNG_FLUG_W / LEISTUNG_LAUFEN_W / LEISTUNG_STEHEN_W as P.
// ---------------------------------------------------------------------------
float energie_verbrauch(float leistung_w, float dt) {
    return leistung_w * dt;
}

// ---------------------------------------------------------------------------
// energie_aufnahme
//
// A bee eating zucker_g grams of sugar gains ENERGIE_DICHTE_J_G joules/g.
// ---------------------------------------------------------------------------
float energie_aufnahme(float zucker_g) {
    return zucker_g * ENERGIE_DICHTE_J_G;
}

// ---------------------------------------------------------------------------
// wachs_aus_zucker
//
// Wax secretion is metabolically expensive: ~8 mg wax per g sugar burned.
// (Real bees need ~6-8 kg honey to produce 1 kg wax.)
// ---------------------------------------------------------------------------
float wachs_aus_zucker(float zucker_g) {
    return zucker_g * WACHS_AUSBEUTE;
}

// ---------------------------------------------------------------------------
// nektar_verdunstung
//
// Fanning bees accelerate water evaporation from uncapped nectar.
// Base rate + fan_factor bonus per active fanning bee.
//
//   wasser_weg_g = VERDUNSTUNG_BASIS * (1 + FAECHER_SKALIERUNG * n_faecher) * dt
// ---------------------------------------------------------------------------
float nektar_verdunstung(int faecher_anzahl, float dt) {
    float fan_faktor = 1.0f + FAECHER_SKALIERUNG * (float)faecher_anzahl;
    return VERDUNSTUNG_BASIS * fan_faktor * dt;
}

// ---------------------------------------------------------------------------
// patch_profit
//
// A forager only recruits others if the patch is profitable.
// Profit = sugar income - flight energy cost, scaled by survival chance.
//
//   kosten_g  = flight_energy_J / ENERGIE_DICHTE_J_G  (sugar equivalent)
//   profit    = (zucker_pro_min - kosten_g_pro_min) * (1 - risiko)
//
// Returns grams of sugar profit per minute. Negative = not worth visiting.
// ---------------------------------------------------------------------------
float patch_profit(float zucker_pro_min, float flugzeit_s, float risiko) {
    // Convert flight energy to sugar equivalent (round trip = 2x)
    float flugenergie_j     = LEISTUNG_FLUG_W * flugzeit_s * 2.0f;
    float flugkosten_g_min  = (flugenergie_j / ENERGIE_DICHTE_J_G)
                              / (flugzeit_s / 60.0f);   // Normalize to per-minute
    float ueberschuss       = zucker_pro_min - flugkosten_g_min;
    return ueberschuss * (1.0f - risiko);
}

// ---------------------------------------------------------------------------
// rekrutierung
//
// Waggle dance recruitment follows a saturating curve (Michaelis-Menten):
//   neue_sammler = R_max * profit / (profit + K)
//
// At very high profit → approaches max_rekrutierung.
// At profit = skalierung_k → half of max recruited.
// At profit <= 0 → no recruitment.
// ---------------------------------------------------------------------------
int rekrutierung(float profit, float max_rekrutierung, float skalierung_k) {
    if (profit <= 0.0f) return 0;
    float anteil = profit / (profit + skalierung_k);
    return (int)(max_rekrutierung * anteil);
}

// ---------------------------------------------------------------------------
// brut_sterblichkeit
//
// Optimal temperature: 34-36°C. Deviation increases mortality.
// nahrungs_mangel: 0.0 = well fed, 1.0 = starving.
//
// Returns probability that a brood cell dies this tick (0.0 - 1.0).
// ---------------------------------------------------------------------------
float brut_sterblichkeit(float temperatur_c, float nahrungs_mangel) {
    float basis = 0.0001f;  // 0.01% base mortality per tick at optimal conditions

    // Temperature stress: rises quadratically outside 34-36°C
    float temp_abweichung = temperatur_c - 35.0f;
    float temp_stress     = temp_abweichung * temp_abweichung * 0.002f;

    // Food stress: linear contribution
    float nahrungsstress = nahrungs_mangel * 0.01f;

    float p = basis + temp_stress + nahrungsstress;
    if (p > 1.0f) p = 1.0f;
    return p;
}

// ---------------------------------------------------------------------------
// haushalt_selbsttest
//
// Spot-checks with known biological values.
// Call once at startup in debug builds — crashes loudly if formulas are wrong.
// ---------------------------------------------------------------------------
void haushalt_selbsttest(void) {
    // 100g nectar at 30% sugar → ~36.6g honey
    float honig = honig_aus_nektar(100.0f, ZUCKERGEHALT_NEKTAR);
    assert(honig > 36.0f && honig < 37.0f);

    // 1 minute flight at LEISTUNG_FLUG_W → 0.014 * 60 = 0.84J
    float e = energie_verbrauch(LEISTUNG_FLUG_W, 60.0f);
    assert(e > 0.83f && e < 0.85f);

    // 1g sugar → 17J
    float aufnahme = energie_aufnahme(1.0f);
    assert(aufnahme > 16.9f && aufnahme < 17.1f);

    // 1g sugar → 8mg wax
    float wachs = wachs_aus_zucker(1.0f);
    assert(wachs > 7.9f && wachs < 8.1f);

    // Negative profit → zero recruits
    assert(rekrutierung(-1.0f, MAX_REKRUTIERUNG, REKRUTIERUNG_K) == 0);

    // Very high profit → close to max recruits
    int r = rekrutierung(100.0f, 10.0f, 0.5f);
    assert(r >= 9 && r <= 10);

    // At 35°C, no food stress → near-zero mortality
    float p = brut_sterblichkeit(35.0f, 0.0f);
    assert(p < 0.001f);

    // At 20°C (cold stress) → significant mortality
    float p_kalt = brut_sterblichkeit(20.0f, 0.0f);
    assert(p_kalt > 0.1f);

    printf("haushalt_selbsttest: alle Formeln korrekt.\n");
}
