#ifndef BIENE_H
#define BIENE_H

#include <stdbool.h>

// Development stages: egg → larva → pupa → adult
typedef enum {
    BIEN_EI = 0,
    BIEN_LARVE,
    BIEN_PUPPE,
    BIEN_ADULT,
} BienZustand;

// Roles of adult worker bees (change with age and colony need)
typedef enum {
    PUTZERIN = 0,        // Cleans cells
    AMME,                // Feeds larvae
    BAUERIN,             // Builds comb
    EMPFAENGERIN,        // Receives and processes nectar
    FAECHER,             // Fans air, evaporates water
    WAECHTER,            // Guards the entrance
    BESTATERIN,          // Removes dead bees
    SAMMLERIN_NEKTAR,    // Forages nectar
    SAMMLERIN_POLLEN,    // Forages pollen
    SAMMLERIN_WASSER,    // Fetches water
    SAMMLERIN_PROPOLIS,  // Fetches propolis/resin
    ERKUNDERIN,          // Scouts new forage patches
} BienRolle;

// Movement / activity states (state machine per bee)
typedef enum {
    IM_STOCK_ARBEITEND = 0,  // Executing an indoor job
    ZUM_AUSGANG,             // Walking toward the entrance
    AUSSENFLUG,              // Flying to a forage target
    AM_SAMMELN,              // Actively collecting resource
    HEIMFLUG,                // Flying back to hive
    ZUM_ABLADEN,             // Searching for drop-off point inside
    RASTEN,                  // Resting, recovering energy
} BienBewegung;

// Full bee data — one entry in the flat bees array
typedef struct {
    BienZustand  zustand;
    BienRolle    rolle;
    BienBewegung bewegung;
    float alter_t;           // Age in days
    float energie_j;         // Energy reserve (sugar as fuel)
    float honigmagen_mg;     // Honey stomach fill (nectar in mg)
    float pollenladung_mg;   // Pollen load on hind legs (mg)
    float wasser_mg;         // Water carried (mg)
    float gesundheit;        // Health (0.0 = dead, 1.0 = perfect)
    float pos_x, pos_y;      // Current position (pixels)
    float ziel_x, ziel_y;    // Current movement target
    int   auftrag_id;        // Index into job queue (-1 = no job)
    float arbeitszeit;       // Time accumulated at current job location (seconds)
    bool  aktiv;             // false = dead (slot reusable)
} Biene;

#endif // BIENE_H
