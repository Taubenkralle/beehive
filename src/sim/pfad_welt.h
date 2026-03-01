#ifndef PFAD_WELT_H
#define PFAD_WELT_H

#include "raylib.h"
#include "../kern/spielzustand.h"
#include "pfad_stock.h"   // Re-uses PFAD_FF_BREITE / PFAD_FF_HOEHE / PFAD_FF_ZELLEN

// ---------------------------------------------------------------------------
// Public API — outdoor flowfields (ANSICHT_WIESE)
// ---------------------------------------------------------------------------

// Compute all outdoor flowfields: one per active source + one for hive entrance.
// Call at startup and whenever sources move/appear/disappear.
void    pfad_welt_init(const Spielzustand* spiel);

// Recompute the flowfield that leads to forage source [quellen_id]
void    pfad_welt_berechnen(int quellen_id, const Spielzustand* spiel);

// Recompute the "come home" flowfield that leads bees back to the hive entrance
void    pfad_eingang_berechnen(void);

// Query direction toward forage source [quellen_id] from position (x,y)
// Returns (0,0) if quellen_id is out of range or not yet computed
Vector2 pfad_welt_richtung(float pos_x, float pos_y, int quellen_id);

// Query direction toward the hive entrance from position (x,y)
Vector2 pfad_eingang_richtung(float pos_x, float pos_y);

#endif // PFAD_WELT_H
