#ifndef AUFTRAG_H
#define AUFTRAG_H

#include <stdbool.h>

// All job types the colony can generate and assign to bees
typedef enum {
    // Indoor jobs
    AUFTRAG_ZELLE_REINIGEN = 0,  // Clean an empty cell
    AUFTRAG_LARVE_FUETTERN,      // Feed a larva (honey + pollen)
    AUFTRAG_WABE_BAUEN,          // Build comb (consumes wax)
    AUFTRAG_NEKTAR_VERARBEITEN,  // Process nectar → honey (fan + enzyme)
    AUFTRAG_FAECHELN,            // Fan air in a zone (cooling / evaporation)
    AUFTRAG_EINGANG_BEWACHEN,    // Guard the hive entrance
    AUFTRAG_LEICHE_ENTFERNEN,    // Remove a dead bee from the hive
    // Outdoor jobs
    AUFTRAG_NEKTAR_SAMMELN,      // Forage nectar from a patch
    AUFTRAG_POLLEN_SAMMELN,      // Forage pollen from a patch
    AUFTRAG_WASSER_HOLEN,        // Fetch water from a source
    AUFTRAG_PROPOLIS_HOLEN,      // Fetch propolis/resin
    AUFTRAG_ERKUNDEN,            // Scout for new forage patches
} AuftragsTyp;

// A single job entry in the colony's job queue
typedef struct {
    AuftragsTyp typ;
    int   ziel_zelle;    // Target cell index in flat array (-1 if unused)
    float ziel_x;        // Target world position x
    float ziel_y;        // Target world position y
    int   prioritaet;    // Priority (1 = highest, higher numbers = lower)
    bool  vergeben;      // Already claimed by a bee
    bool  aktiv;         // Slot in use (false = free for reuse by new jobs)
} Auftrag;

#endif // AUFTRAG_H
