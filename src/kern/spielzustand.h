#ifndef SPIELZUSTAND_H
#define SPIELZUSTAND_H

#include "raylib.h"
#include "zelle.h"
#include "biene.h"
#include "auftrag.h"
#include "trachtquelle.h"
#include "../sim/pheromon.h"

// ---------------------------------------------------------------------------
// Window & timing
// ---------------------------------------------------------------------------
#define FENSTER_BREITE  1280
#define FENSTER_HOEHE   720
#define ZIEL_FPS        60

// ---------------------------------------------------------------------------
// Flat array limits (performance: no malloc per bee/cell)
// ---------------------------------------------------------------------------
#define MAX_BIENEN      1000   // Max simultaneous bees (increase later)
#define MAX_ZELLEN      578    // 34 columns * 17 rows (hex grid)
#define MAX_AUFTRAEGE   256    // Job queue capacity
#define MAX_QUELLEN     64     // Forage patches + water/propolis sources

// ---------------------------------------------------------------------------
// Views
// ---------------------------------------------------------------------------
typedef enum {
    ANSICHT_STOCK,   // Cross-section: inside the hive
    ANSICHT_WIESE,   // Top-down: outside meadow world
} Ansicht;

// ---------------------------------------------------------------------------
// Full game + simulation state
// ---------------------------------------------------------------------------
typedef struct {
    // View
    Ansicht ansicht;
    float   zeit;              // Elapsed time in seconds

    // Resources (real physical units)
    float honig_reif_g;        // Ripe honey (grams)
    float nektar_unreif_g;     // Nectar being processed (grams)
    float pollen_g;            // Pollen reserve (grams)
    float wasser_g;            // Water reserve (grams)
    float wachs_mg;            // Wax reserve (milligrams)

    // Brood counts per stage
    int brut_ei;
    int brut_larve;
    int brut_puppe;

    // Climate — inside hive
    float temperatur_c;        // Brood nest temperature (target: 35°C)
    float bedrohungslevel;     // Alarm pheromone level at entrance (0.0-1.0)

    // Weather — outside world
    float tageszeit;           // Time of day: 0.0=midnight, 0.5=noon, 1.0=midnight
    float aussentemp_c;        // Outside air temperature (°C)
    float wind_staerke;        // Wind speed 0.0 (calm) to 1.0 (storm)
    bool  regen;               // Rain active (blocks all flight)
    float wetter_timer;        // Countdown in seconds until next weather check
    bool  flug_gesperrt;       // True when weather prevents foraging

    // Flat simulation arrays (cache-friendly, no heap allocation)
    Biene        bienen[MAX_BIENEN];
    Zelle        zellen[MAX_ZELLEN];
    Auftrag      auftraege[MAX_AUFTRAEGE];
    Trachtquelle quellen[MAX_QUELLEN];

    // Active counts
    int bienen_anzahl;
    int zellen_anzahl;
    int auftraege_anzahl;
    int quellen_anzahl;

    // Pheromone fields (one grid per type, covers full window)
    PheromonFeld pheromonfelder[PHEROMON_ANZAHL];
    bool         pheromon_debug;   // Toggle heatmap overlay with [P]

    // Legacy display fields (used by renderer until Phase 8 integration)
    int   volksgroesse;        // Visible bee count for HUD
    float honig;               // Honey level 0.0-1.0 for HUD
    float pollen;              // Pollen level 0.0-1.0 for HUD
} Spielzustand;

#endif // SPIELZUSTAND_H
