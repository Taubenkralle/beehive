#ifndef TRACHTQUELLE_H
#define TRACHTQUELLE_H

#include <stdbool.h>

// Resource types a forage source can provide
typedef enum {
    TRACHT_NEKTAR = 0,
    TRACHT_POLLEN,
    TRACHT_WASSER,
    TRACHT_PROPOLIS,
} TrachtTyp;

// A forage patch or resource point in the outside world
typedef struct {
    TrachtTyp typ;
    float pos_x, pos_y;         // World position
    float vorrat_g;             // Current resource amount (grams)
    float kapazitaet_g;         // Maximum capacity
    float regeneration_g_s;     // Refill rate per second
    float risiko;               // Danger level (0.0 = safe, 1.0 = deadly)
    bool  aktiv;                // Visible and accessible to bees
} Trachtquelle;

#endif // TRACHTQUELLE_H
