#ifndef ZELLE_H
#define ZELLE_H

#include <stdbool.h>

// All cell types a honeycomb slot can hold
typedef enum {
    ZELLE_LEER = 0,       // Empty, available
    ZELLE_BRUT_EI,        // Egg (day 1-3)
    ZELLE_BRUT_LARVE,     // Larva (day 4-9)
    ZELLE_BRUT_PUPPE,     // Capped pupa (day 10-21)
    ZELLE_HONIG_OFFEN,    // Uncapped honey / nectar ripening
    ZELLE_HONIG_VERD,     // Capped ripe honey
    ZELLE_POLLEN,         // Pollen storage
    ZELLE_WACHSBAU,       // Under construction (wax being added)
    ZELLE_KOENIGIN,       // Queen area
    ZELLE_BLOCKIERT,      // Impassable (structural)
} ZellenTyp;

// Full cell data — used by the simulation
typedef struct {
    ZellenTyp typ;
    float     inhalt;     // Fill level (0.0 = empty, 1.0 = full)
    float     alter_t;    // Age of content in days
    bool      sauber;     // Clean and ready for egg-laying
} Zelle;

#endif // ZELLE_H
