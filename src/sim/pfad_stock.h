#ifndef PFAD_STOCK_H
#define PFAD_STOCK_H

#include "raylib.h"
#include "../kern/spielzustand.h"

// ---------------------------------------------------------------------------
// Flowfield grid — covers the full hive cross-section view (ANSICHT_STOCK)
// 20×20 pixel cells → 64×36 grid
// ---------------------------------------------------------------------------
#define PFAD_ZELLEN_GROESSE  20             // Pixels per flowfield cell
#define PFAD_FF_BREITE       64             // FENSTER_BREITE / PFAD_ZELLEN_GROESSE
#define PFAD_FF_HOEHE        36             // FENSTER_HOEHE  / PFAD_ZELLEN_GROESSE
#define PFAD_FF_ZELLEN       (PFAD_FF_BREITE * PFAD_FF_HOEHE)   // 2304

// ---------------------------------------------------------------------------
// Target zones inside the hive (each gets its own flowfield)
// ---------------------------------------------------------------------------
typedef enum {
    STOCK_ZONE_EINGANG = 0,   // Hive entrance → bees leaving
    STOCK_ZONE_LAGER,         // Drop-off zone → returning foragers
    STOCK_ZONE_BRUT,          // Brood area → nurses, cleaners
    STOCK_ZONE_HONIG,         // Honey storage → nectar processors
    STOCK_ZONE_POLLEN,        // Pollen storage → pollen foragers
    STOCK_ZONE_ANZAHL
} StockZone;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Compute all indoor flowfields (call once at startup; no runtime obstacle changes yet)
void    pfad_stock_init(void);

// Recompute flowfield for one zone (if zone geometry ever changes)
void    pfad_stock_berechnen(StockZone zone);

// Query: returns the movement direction a bee at screen position (x,y) should take
// to reach the given zone. Returns (0,0) if position is out of bounds.
Vector2 pfad_stock_richtung(float pos_x, float pos_y, StockZone zone);

#endif // PFAD_STOCK_H
