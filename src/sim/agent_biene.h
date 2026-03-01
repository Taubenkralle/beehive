#ifndef AGENT_BIENE_H
#define AGENT_BIENE_H

#include "../kern/spielzustand.h"

// ---------------------------------------------------------------------------
// World anchor positions (hardcoded for now, wired up in Phase 8)
// ---------------------------------------------------------------------------
#define STOCK_EINGANG_X   130.0f   // Hive entrance in outdoor world (pixels)
#define STOCK_EINGANG_Y   370.0f
#define STOCK_LAGER_X     640.0f   // Indoor drop-off zone (hive cross-section)
#define STOCK_LAGER_Y     360.0f

// ---------------------------------------------------------------------------
// Tuning constants
// ---------------------------------------------------------------------------
#define BIENE_GESCHW_INNEN    45.0f   // Walking speed inside hive (px/s)
#define BIENE_GESCHW_FLUG    160.0f   // Flying speed outside (px/s)
#define BIENE_ZIEL_RADIUS      8.0f   // Distance threshold: "arrived at target"
#define ENERGIE_KRITISCH       0.15f  // Fraction of start energy → emergency mode
#define SAMMELN_RATE_MG_S     12.0f   // mg nectar collected per second at patch
#define ARBEIT_DAUER_S         3.0f   // Seconds to complete one indoor job action
#define MAX_HONIGMAGEN_MG     40.0f   // Honey stomach capacity
#define MAX_POLLENLADUNG_MG   15.0f   // Pollen basket capacity

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Initialize a bee at position (x,y) with given role and full energy
void  biene_init(Biene* b, float x, float y, BienRolle rolle);

// Main per-tick update: Sense → Decide → Act → PayCost
void  biene_aktualisieren(Biene* b, Spielzustand* spiel, float dt);

// Batch update: call biene_aktualisieren for all active adult bees
void  alle_bienen_aktualisieren(Spielzustand* spiel, float dt);

// Choose role based on bee age and colony need (called by sim_stock)
void  biene_rolle_waehlen(Biene* b, const Spielzustand* spiel);

// Claim highest-priority matching job from the colony job queue
void  biene_auftrag_holen(Biene* b, Spielzustand* spiel);

// Steer toward current target using flowfield guidance (falls back to direct steering)
void  biene_bewegen(Biene* b, const Spielzustand* spiel, float dt);

// Deduct energy cost for performing action over dt seconds
void  biene_energie_verbrauchen(Biene* b, float leistung_w, float dt);

// Load resource into bee's body (nectar → honigmagen, pollen → pollenladung)
void  biene_ressource_laden(Biene* b, TrachtTyp typ, float menge_mg);

// Deposit all carried resources into colony stores (spiel->nektar_unreif_g etc.)
void  biene_ressource_abladen(Biene* b, Spielzustand* spiel);

// Emit pheromone at current position
void  biene_pheromon_abgeben(Biene* b, PheromonFeld* feld, PheromonTyp typ, float dt);

// True if bee has run out of energy or health
bool  biene_ist_tot(const Biene* b);

#endif // AGENT_BIENE_H
