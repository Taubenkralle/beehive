#ifndef SIM_STOCK_H
#define SIM_STOCK_H

#include "../kern/spielzustand.h"

// ---------------------------------------------------------------------------
// Zone center coordinates for job targets
// These approximate hive-view positions; Phase 8 will wire up exact cell coords.
// ---------------------------------------------------------------------------
#define ZONE_BRUT_X       400.0f   // Brood area center
#define ZONE_BRUT_Y       360.0f
#define ZONE_HONIG_X      680.0f   // Honey storage area
#define ZONE_HONIG_Y      250.0f
#define ZONE_POLLEN_X     680.0f   // Pollen storage area
#define ZONE_POLLEN_Y     470.0f
#define ZONE_EINGANG_X    180.0f   // Inner side of hive entrance
#define ZONE_EINGANG_Y    370.0f
#define ZONE_WABENBAU_X   540.0f   // Comb-building zone
#define ZONE_WABENBAU_Y   300.0f

// ---------------------------------------------------------------------------
// Colony-level tuning constants
// ---------------------------------------------------------------------------

// Nectar → honey conversion rate per fanning bee (grams/second)
#define FAECHER_NEKTAR_RATE_G_S   0.0005f

// Wax production rate per Bauerin bee (grams honey consumed/second)
#define BIENE_WACHSPROD_G_S       0.00005f

// Queen lays this many eggs per simulated day (at full health)
#define KOENIGIN_EIER_PRO_TAG     200.0f

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Initialize simulation state: resources, brood cells, starting bees
void stock_init(Spielzustand* spiel);

// Main colony tick: calls all sub-systems in correct order
void stock_aktualisieren(Spielzustand* spiel, float dt);

// Generate jobs based on current colony needs (demand → job queue)
void stock_auftraege_generieren(Spielzustand* spiel);

// Advance brood development: Ei → Larve → Puppe → hatched bee
void stock_brut_aktualisieren(Spielzustand* spiel, float dt);

// Evaporate water from unripe nectar, convert ripe batches to honey
void stock_nektar_verarbeiten(Spielzustand* spiel, float dt);

// Regulate brood nest temperature toward 35°C
void stock_thermoregulieren(Spielzustand* spiel, float dt);

// Metabolize honey into wax; convert accumulated wax into new comb cells
void stock_wabe_bauen(Spielzustand* spiel, float dt);

#endif // SIM_STOCK_H
