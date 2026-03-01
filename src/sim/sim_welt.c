#include "sim_welt.h"
#include <math.h>
#include <stdlib.h>

// Linearly interpolate between a and b using t in [0,1]
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Clamp x to [lo, hi]
static float klemmen(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ---------------------------------------------------------------------------
// welt_init
// ---------------------------------------------------------------------------
void welt_init(Spielzustand* spiel) {
    // Start at 6 am (good time to begin watching bees)
    spiel->tageszeit     = 0.25f;
    spiel->aussentemp_c  = AUSSENTEMP_BASIS_C + 2.0f;
    spiel->wind_staerke  = 0.1f;
    spiel->regen         = false;
    spiel->wetter_timer  = WETTER_INTERVALL_S;
    spiel->flug_gesperrt = false;

    // Forage patches in the outdoor world (ANSICHT_WIESE pixel coords)
    spiel->quellen_anzahl = 0;

    // Patch 1 — Nektar: large flower meadow to the east
    spiel->quellen[spiel->quellen_anzahl++] = (Trachtquelle){
        TRACHT_NEKTAR,
        .pos_x          = 750.0f,
        .pos_y          = 220.0f,
        .vorrat_g       = 150.0f,   // Partially stocked at start
        .kapazitaet_g   = 400.0f,
        .regeneration_g_s = 0.08f,  // g/s at peak conditions
        .risiko         = 0.05f,
        .aktiv          = true
    };

    // Patch 2 — Pollen: wildflower strip to the north-east
    spiel->quellen[spiel->quellen_anzahl++] = (Trachtquelle){
        TRACHT_POLLEN,
        .pos_x          = 950.0f,
        .pos_y          = 380.0f,
        .vorrat_g       = 60.0f,
        .kapazitaet_g   = 200.0f,
        .regeneration_g_s = 0.04f,
        .risiko         = 0.03f,
        .aktiv          = true
    };

    // Patch 3 — Wasser: small pond to the south
    spiel->quellen[spiel->quellen_anzahl++] = (Trachtquelle){
        TRACHT_WASSER,
        .pos_x          = 680.0f,
        .pos_y          = 560.0f,
        .vorrat_g       = 1000.0f,  // Nearly inexhaustible
        .kapazitaet_g   = 1000.0f,
        .regeneration_g_s = 0.5f,
        .risiko         = 0.01f,
        .aktiv          = true
    };

    // Patch 4 — Nektar: second smaller flower cluster
    spiel->quellen[spiel->quellen_anzahl++] = (Trachtquelle){
        TRACHT_NEKTAR,
        .pos_x          = 1050.0f,
        .pos_y          = 180.0f,
        .vorrat_g       = 80.0f,
        .kapazitaet_g   = 250.0f,
        .regeneration_g_s = 0.05f,
        .risiko         = 0.08f,
        .aktiv          = true
    };
}

// ---------------------------------------------------------------------------
// flug_moeglich
// ---------------------------------------------------------------------------
bool flug_moeglich(const Spielzustand* spiel) {
    if (spiel->regen)                          return false;
    if (spiel->aussentemp_c < FLUG_TEMP_MIN_C) return false;
    if (spiel->wind_staerke  > FLUG_WIND_MAX)  return false;
    // Also check time: bees don't fly at night (tageszeit near 0 or 1)
    if (spiel->tageszeit < 0.10f || spiel->tageszeit > 0.90f) return false;
    return true;
}

// ---------------------------------------------------------------------------
// trachtquelle_aktualisieren
// ---------------------------------------------------------------------------
void trachtquelle_aktualisieren(Trachtquelle* q, const Spielzustand* spiel, float dt) {
    if (!q->aktiv) return;
    if (q->vorrat_g >= q->kapazitaet_g) return;   // Already full

    // Time-of-day multiplier: peaks at noon, zero at night
    // tageszeit: 0=midnight, 0.5=noon → sin(t*π) gives smooth bell from 0 to 1 and back
    float tageszeit_faktor = sinf(spiel->tageszeit * 3.14159f);
    if (tageszeit_faktor < 0.0f) tageszeit_faktor = 0.0f;

    // Temperature factor: optimal 15–30°C, drops off outside that range
    float temp_faktor = 1.0f;
    if (spiel->aussentemp_c < 12.0f)
        temp_faktor = klemmen((spiel->aussentemp_c - 5.0f) / 7.0f, 0.0f, 1.0f);
    else if (spiel->aussentemp_c > 32.0f)
        temp_faktor = klemmen(1.0f - (spiel->aussentemp_c - 32.0f) / 8.0f, 0.0f, 1.0f);

    // Rain halts nectar/pollen production, water source stays open
    float regen_faktor = (spiel->regen && q->typ != TRACHT_WASSER) ? 0.0f : 1.0f;

    // Wind slightly reduces nectar (evaporates it) but not pollen
    float wind_faktor = (q->typ == TRACHT_NEKTAR)
                        ? (1.0f - spiel->wind_staerke * 0.2f) : 1.0f;

    float rate = q->regeneration_g_s
                 * tageszeit_faktor * temp_faktor * regen_faktor * wind_faktor;

    q->vorrat_g += rate * dt;
    if (q->vorrat_g > q->kapazitaet_g) q->vorrat_g = q->kapazitaet_g;
}

// ---------------------------------------------------------------------------
// wetter_aktualisieren
// ---------------------------------------------------------------------------
void wetter_aktualisieren(Spielzustand* spiel, float dt) {
    // --- Wind: slow continuous drift using sinusoidal variation ---
    // Two overlapping sine waves with slightly different periods give
    // naturalistic, non-repeating wind variation without rand().
    float wind_base = 0.5f + 0.4f * sinf(spiel->zeit * 0.017f)
                           + 0.1f * sinf(spiel->zeit * 0.053f);
    // Smoothly approach the target (low-pass filter)
    spiel->wind_staerke = lerp(spiel->wind_staerke, wind_base, 0.3f * dt);
    spiel->wind_staerke  = klemmen(spiel->wind_staerke, 0.0f, 1.0f);

    // --- Rain: timer-based state changes ---
    spiel->wetter_timer -= dt;
    if (spiel->wetter_timer <= 0.0f) {
        spiel->wetter_timer = WETTER_INTERVALL_S;

        if (spiel->regen) {
            // Rain clears up ~30% of the time each check
            if (rand() % 10 < 3) {
                spiel->regen = false;
                // After rain: wind calms down
                spiel->wind_staerke *= 0.5f;
            }
        } else {
            // Rain starts ~10% of the time during windy conditions
            float rain_chance = spiel->wind_staerke > 0.6f ? 0.15f : 0.05f;
            if ((float)(rand() % 100) / 100.0f < rain_chance)
                spiel->regen = true;
        }
    }

    // --- Outside temperature: day/night sine + weather effects ---
    float temp_ziel = AUSSENTEMP_BASIS_C
                     + AUSSENTEMP_AMPLITUDE * sinf(spiel->tageszeit * 3.14159f);
    if (spiel->regen)  temp_ziel -= 5.0f;    // Rain cools things down
    if (spiel->wind_staerke > 0.5f) temp_ziel -= 2.0f;  // Wind chill
    // Drift toward target (slow thermal lag)
    spiel->aussentemp_c = lerp(spiel->aussentemp_c, temp_ziel, 0.01f * dt);

    // --- Update flight lock ---
    spiel->flug_gesperrt = !flug_moeglich(spiel);
}

// ---------------------------------------------------------------------------
// welt_aktualisieren
// ---------------------------------------------------------------------------
void welt_aktualisieren(Spielzustand* spiel, float dt) {
    // Advance time of day (wraps 0→1 each simulated day)
    spiel->tageszeit += dt / SIM_TAG_DAUER_S;
    if (spiel->tageszeit >= 1.0f) spiel->tageszeit -= 1.0f;

    // Advance elapsed-time clock
    spiel->zeit += dt;

    wetter_aktualisieren(spiel, dt);

    for (int i = 0; i < spiel->quellen_anzahl; i++)
        trachtquelle_aktualisieren(&spiel->quellen[i], spiel, dt);
}
