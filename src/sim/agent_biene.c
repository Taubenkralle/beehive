#include "agent_biene.h"
#include "haushalt.h"
#include "pfad_stock.h"
#include "pfad_welt.h"
#include <math.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// True if bee has reached its current movement target
static bool am_ziel(const Biene* b) {
    float dx = b->ziel_x - b->pos_x;
    float dy = b->ziel_y - b->pos_y;
    return (dx*dx + dy*dy) < (BIENE_ZIEL_RADIUS * BIENE_ZIEL_RADIUS);
}

// Power consumption for each movement state
static float leistung_fuer_bewegung(BienBewegung bew) {
    switch (bew) {
        case AUSSENFLUG:
        case HEIMFLUG:          return LEISTUNG_FLUG_W;
        case IM_STOCK_ARBEITEND:
        case ZUM_AUSGANG:
        case ZUM_ABLADEN:       return LEISTUNG_LAUFEN_W;
        case AM_SAMMELN:        return LEISTUNG_LAUFEN_W;  // Hovering at flower
        case RASTEN:            return LEISTUNG_STEHEN_W;
        default:                return LEISTUNG_STEHEN_W;
    }
}

// True if a job type requires leaving the hive
static bool auftrag_ist_aussen(AuftragsTyp typ) {
    return typ == AUFTRAG_NEKTAR_SAMMELN
        || typ == AUFTRAG_POLLEN_SAMMELN
        || typ == AUFTRAG_WASSER_HOLEN
        || typ == AUFTRAG_PROPOLIS_HOLEN
        || typ == AUFTRAG_ERKUNDEN;
}

// Natural role progression by age (matches real bee biology)
static BienRolle rolle_fuer_alter(float alter_t) {
    if (alter_t <  3.0f) return PUTZERIN;
    if (alter_t < 10.0f) return AMME;
    if (alter_t < 16.0f) return BAUERIN;
    if (alter_t < 20.0f) return EMPFAENGERIN;
    if (alter_t < 24.0f) return FAECHER;
    return SAMMLERIN_NEKTAR;  // Forager in final weeks of life
}

// Map a role to its preferred job type
static AuftragsTyp bevorzugter_auftrag(BienRolle rolle) {
    switch (rolle) {
        case PUTZERIN:         return AUFTRAG_ZELLE_REINIGEN;
        case AMME:             return AUFTRAG_LARVE_FUETTERN;
        case BAUERIN:          return AUFTRAG_WABE_BAUEN;
        case EMPFAENGERIN:     return AUFTRAG_NEKTAR_VERARBEITEN;
        case FAECHER:          return AUFTRAG_FAECHELN;
        case WAECHTER:         return AUFTRAG_EINGANG_BEWACHEN;
        case BESTATERIN:       return AUFTRAG_LEICHE_ENTFERNEN;
        case SAMMLERIN_NEKTAR: return AUFTRAG_NEKTAR_SAMMELN;
        case SAMMLERIN_POLLEN: return AUFTRAG_POLLEN_SAMMELN;
        case SAMMLERIN_WASSER: return AUFTRAG_WASSER_HOLEN;
        case SAMMLERIN_PROPOLIS: return AUFTRAG_PROPOLIS_HOLEN;
        case ERKUNDERIN:       return AUFTRAG_ERKUNDEN;
        default:               return AUFTRAG_ZELLE_REINIGEN;
    }
}

// Release current job back to the queue (so another bee can take it)
static void auftrag_freigeben(Biene* b, Spielzustand* spiel) {
    if (b->auftrag_id >= 0 && b->auftrag_id < spiel->auftraege_anzahl) {
        spiel->auftraege[b->auftrag_id].vergeben = false;
        spiel->auftraege[b->auftrag_id].aktiv    = false;  // Slot free for reuse
    }
    b->auftrag_id  = -1;
    b->arbeitszeit = 0.0f;
}

// Collect resource from nearest matching forage source at current position
static void biene_sammeln(Biene* b, Spielzustand* spiel, float dt) {
    float beste_dist = 60.0f * 60.0f;  // Search radius squared (60px)
    int   quelle_idx = -1;

    for (int i = 0; i < spiel->quellen_anzahl; i++) {
        Trachtquelle* q = &spiel->quellen[i];
        if (!q->aktiv || q->vorrat_g <= 0.0f) continue;

        float dx = q->pos_x - b->pos_x;
        float dy = q->pos_y - b->pos_y;
        float d2 = dx*dx + dy*dy;
        if (d2 < beste_dist) {
            beste_dist = d2;
            quelle_idx = i;
        }
    }

    if (quelle_idx < 0) return;  // No source nearby

    Trachtquelle* q = &spiel->quellen[quelle_idx];
    float menge = SAMMELN_RATE_MG_S * dt / 1000.0f;  // mg → g
    if (menge > q->vorrat_g) menge = q->vorrat_g;

    q->vorrat_g -= menge;
    biene_ressource_laden(b, q->typ, menge * 1000.0f);  // g → mg

    // Emit trail pheromone while foraging
    biene_pheromon_abgeben(b, &spiel->pheromonfelder[PHEROMON_SPUR],
                           PHEROMON_SPUR, dt);
}

// Execute an indoor job action — timed: completes after ARBEIT_DAUER_S seconds
static void biene_auftrag_ausfuehren(Biene* b, Spielzustand* spiel, float dt) {
    b->arbeitszeit += dt;
    if (b->arbeitszeit < ARBEIT_DAUER_S) return;  // Still working

    // Action completed — apply effect
    if (b->auftrag_id >= 0) {
        AuftragsTyp typ = spiel->auftraege[b->auftrag_id].typ;

        switch (typ) {
            case AUFTRAG_LARVE_FUETTERN:
                // Consume colony stocks to feed a larva
                if (spiel->honig_reif_g > 0.01f) spiel->honig_reif_g -= 0.01f;
                if (spiel->pollen_g     > 0.005f) spiel->pollen_g    -= 0.005f;
                break;

            case AUFTRAG_NEKTAR_VERARBEITEN:
                // Fan to ripen nectar — handled by sim_stock, just mark done
                break;

            case AUFTRAG_WABE_BAUEN:
                // Consume wax to build (sim_stock tracks cell creation)
                if (spiel->wachs_mg > WACHS_PRO_ZELLE_MG)
                    spiel->wachs_mg -= WACHS_PRO_ZELLE_MG;
                break;

            case AUFTRAG_FAECHELN:
                // Evaporate water from nectar (sim_stock applies full result)
                break;

            default:
                break;  // Other jobs: time cost only
        }
    }

    auftrag_freigeben(b, spiel);
    b->bewegung = IM_STOCK_ARBEITEND;
}

// ---------------------------------------------------------------------------
// biene_init
// ---------------------------------------------------------------------------
void biene_init(Biene* b, float x, float y, BienRolle rolle) {
    memset(b, 0, sizeof(Biene));
    b->zustand       = BIEN_ADULT;
    b->rolle         = rolle;
    b->bewegung      = IM_STOCK_ARBEITEND;
    b->pos_x         = x;
    b->pos_y         = y;
    b->ziel_x        = x;
    b->ziel_y        = y;
    b->energie_j     = ENERGIE_START_J;
    b->gesundheit    = 1.0f;
    b->auftrag_id    = -1;
    b->arbeitszeit   = 0.0f;
    b->aktiv         = true;
}

// ---------------------------------------------------------------------------
// biene_bewegen — flowfield-guided steering with direct-steering fallback
// ---------------------------------------------------------------------------

// Map bee role to the indoor zone it wants to reach
static StockZone zone_fuer_rolle(BienRolle rolle) {
    switch (rolle) {
        case AMME:
        case PUTZERIN:
        case BESTATERIN:   return STOCK_ZONE_BRUT;
        case EMPFAENGERIN: return STOCK_ZONE_HONIG;
        case FAECHER:
        case WAECHTER:     return STOCK_ZONE_EINGANG;
        default:           return STOCK_ZONE_LAGER;
    }
}

// Find which source index a bee is flying toward (matches job target position)
static int quellen_id_fuer_biene(const Biene* b, const Spielzustand* spiel) {
    if (b->auftrag_id < 0) return -1;
    float jx = spiel->auftraege[b->auftrag_id].ziel_x;
    float jy = spiel->auftraege[b->auftrag_id].ziel_y;
    for (int i = 0; i < spiel->quellen_anzahl; i++) {
        float dx = spiel->quellen[i].pos_x - jx;
        float dy = spiel->quellen[i].pos_y - jy;
        if (dx*dx + dy*dy < 16.0f) return i;   // 4px threshold
    }
    return -1;
}

void biene_bewegen(Biene* b, const Spielzustand* spiel, float dt) {
    float dir_x, dir_y;

    // --- Try flowfield first ---
    Vector2 ff = {0, 0};
    switch (b->bewegung) {
        case ZUM_AUSGANG:
            ff = pfad_stock_richtung(b->pos_x, b->pos_y, STOCK_ZONE_EINGANG);
            break;
        case ZUM_ABLADEN:
            ff = pfad_stock_richtung(b->pos_x, b->pos_y, STOCK_ZONE_LAGER);
            break;
        case IM_STOCK_ARBEITEND:
            ff = pfad_stock_richtung(b->pos_x, b->pos_y, zone_fuer_rolle(b->rolle));
            break;
        case AUSSENFLUG: {
            int qid = quellen_id_fuer_biene(b, spiel);
            if (qid >= 0)
                ff = pfad_welt_richtung(b->pos_x, b->pos_y, qid);
            break;
        }
        case HEIMFLUG:
            ff = pfad_eingang_richtung(b->pos_x, b->pos_y);
            break;
        default:
            break;
    }

    if (ff.x != 0.0f || ff.y != 0.0f) {
        // Flowfield gave a valid direction
        dir_x = ff.x;
        dir_y = ff.y;
    } else {
        // Fallback: steer directly toward target
        float dx   = b->ziel_x - b->pos_x;
        float dy   = b->ziel_y - b->pos_y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist < 0.1f) return;
        dir_x = dx / dist;
        dir_y = dy / dist;
    }

    float geschw = (b->bewegung == AUSSENFLUG || b->bewegung == HEIMFLUG)
                   ? BIENE_GESCHW_FLUG : BIENE_GESCHW_INNEN;

    // Don't overshoot the final target
    float dx_rem  = b->ziel_x - b->pos_x;
    float dy_rem  = b->ziel_y - b->pos_y;
    float dist_rem = sqrtf(dx_rem*dx_rem + dy_rem*dy_rem);
    float schritt  = geschw * dt;
    if (schritt > dist_rem) schritt = dist_rem;

    b->pos_x += dir_x * schritt;
    b->pos_y += dir_y * schritt;
}

// ---------------------------------------------------------------------------
// biene_energie_verbrauchen
// ---------------------------------------------------------------------------
void biene_energie_verbrauchen(Biene* b, float leistung_w, float dt) {
    b->energie_j -= energie_verbrauch(leistung_w, dt);
    if (b->energie_j < 0.0f) b->energie_j = 0.0f;
}

// ---------------------------------------------------------------------------
// biene_ressource_laden
// ---------------------------------------------------------------------------
void biene_ressource_laden(Biene* b, TrachtTyp typ, float menge_mg) {
    switch (typ) {
        case TRACHT_NEKTAR:
            b->honigmagen_mg += menge_mg;
            if (b->honigmagen_mg > MAX_HONIGMAGEN_MG)
                b->honigmagen_mg = MAX_HONIGMAGEN_MG;
            break;
        case TRACHT_POLLEN:
            b->pollenladung_mg += menge_mg;
            if (b->pollenladung_mg > MAX_POLLENLADUNG_MG)
                b->pollenladung_mg = MAX_POLLENLADUNG_MG;
            break;
        case TRACHT_WASSER:
            b->wasser_mg += menge_mg;
            break;
        case TRACHT_PROPOLIS:
            break;  // Propolis tracked at colony level
    }
}

// ---------------------------------------------------------------------------
// biene_ressource_abladen — deposit into colony stores
// ---------------------------------------------------------------------------
void biene_ressource_abladen(Biene* b, Spielzustand* spiel) {
    spiel->nektar_unreif_g += b->honigmagen_mg / 1000.0f;  // mg → g
    spiel->pollen_g        += b->pollenladung_mg / 1000.0f;
    spiel->wasser_g        += b->wasser_mg / 1000.0f;

    // Bee also eats a small share of nectar to restore energy
    float zucker_g = (b->honigmagen_mg / 1000.0f) * ZUCKERGEHALT_NEKTAR * 0.05f;
    b->energie_j += energie_aufnahme(zucker_g);
    if (b->energie_j > ENERGIE_START_J) b->energie_j = ENERGIE_START_J;

    b->honigmagen_mg   = 0.0f;
    b->pollenladung_mg = 0.0f;
    b->wasser_mg       = 0.0f;
}

// ---------------------------------------------------------------------------
// biene_pheromon_abgeben
// ---------------------------------------------------------------------------
void biene_pheromon_abgeben(Biene* b, PheromonFeld* feld, PheromonTyp typ, float dt) {
    float menge = 2.0f * dt;  // Emission rate: 2 units/second
    pheromon_abgeben(feld, b->pos_x, b->pos_y, menge);
    (void)typ;  // Type is encoded in which field pointer is passed
}

// ---------------------------------------------------------------------------
// biene_ist_tot
// ---------------------------------------------------------------------------
bool biene_ist_tot(const Biene* b) {
    return b->energie_j <= 0.0f || b->gesundheit <= 0.0f;
}

// ---------------------------------------------------------------------------
// biene_rolle_waehlen — age-based natural progression
// ---------------------------------------------------------------------------
void biene_rolle_waehlen(Biene* b, const Spielzustand* spiel) {
    (void)spiel;  // Colony-level overrides come in Phase 5 (sim_stock)
    BienRolle neue_rolle = rolle_fuer_alter(b->alter_t);
    if (neue_rolle != b->rolle) {
        auftrag_freigeben(b, (Spielzustand*)spiel);  // Drop current job on role change
        b->rolle = neue_rolle;
    }
}

// ---------------------------------------------------------------------------
// biene_auftrag_holen — scan queue for best matching unassigned job
// ---------------------------------------------------------------------------
void biene_auftrag_holen(Biene* b, Spielzustand* spiel) {
    AuftragsTyp ziel_typ = bevorzugter_auftrag(b->rolle);
    int  bester_idx  = -1;
    int  beste_prio  = 999;

    for (int i = 0; i < spiel->auftraege_anzahl; i++) {
        Auftrag* a = &spiel->auftraege[i];
        if (!a->aktiv)                                    continue;
        if (a->vergeben)                                  continue;
        if (a->typ != ziel_typ)                           continue;
        if (auftrag_ist_aussen(a->typ) && spiel->flug_gesperrt) continue;
        if (a->prioritaet < beste_prio) {
            beste_prio  = a->prioritaet;
            bester_idx  = i;
        }
    }

    if (bester_idx < 0) return;  // No matching job available

    Auftrag* a         = &spiel->auftraege[bester_idx];
    a->vergeben        = true;
    b->auftrag_id      = bester_idx;
    b->arbeitszeit     = 0.0f;

    if (auftrag_ist_aussen(a->typ)) {
        // Outdoor job: first walk to hive entrance
        b->bewegung = ZUM_AUSGANG;
        b->ziel_x   = STOCK_EINGANG_X;
        b->ziel_y   = STOCK_EINGANG_Y;
    } else {
        // Indoor job: walk to target cell/zone
        b->bewegung = IM_STOCK_ARBEITEND;
        b->ziel_x   = a->ziel_x;
        b->ziel_y   = a->ziel_y;
    }
}

// ---------------------------------------------------------------------------
// biene_aktualisieren — the full Sense → Decide → Act → PayCost loop
// ---------------------------------------------------------------------------
void biene_aktualisieren(Biene* b, Spielzustand* spiel, float dt) {
    if (!b->aktiv || b->zustand != BIEN_ADULT) return;

    // --- AGE ---
    b->alter_t += dt / 86400.0f;  // Seconds → days

    // --- SENSE: check energy level ---
    bool energie_niedrig = (b->energie_j < ENERGIE_START_J * ENERGIE_KRITISCH);

    // --- DECIDE ---
    // Emergency override: very low energy → interrupt current task
    if (energie_niedrig) {
        if (b->bewegung == AUSSENFLUG || b->bewegung == AM_SAMMELN) {
            // Can't make it inside — emergency return to entrance
            auftrag_freigeben(b, spiel);
            b->bewegung = HEIMFLUG;
            b->ziel_x   = STOCK_EINGANG_X;
            b->ziel_y   = STOCK_EINGANG_Y;
        } else if (b->bewegung == IM_STOCK_ARBEITEND) {
            auftrag_freigeben(b, spiel);
            b->bewegung = RASTEN;
        }
    }

    // Get a job if idle and energy is OK
    if (b->auftrag_id < 0 && !energie_niedrig
        && b->bewegung == IM_STOCK_ARBEITEND) {
        biene_rolle_waehlen(b, spiel);
        biene_auftrag_holen(b, spiel);
    }

    // --- ACT ---
    switch (b->bewegung) {

        case IM_STOCK_ARBEITEND:
            if (b->auftrag_id >= 0) {
                if (am_ziel(b)) {
                    biene_auftrag_ausfuehren(b, spiel, dt);
                } else {
                    biene_bewegen(b, spiel, dt);
                }
            }
            break;

        case ZUM_AUSGANG:
            if (am_ziel(b)) {
                // Arrived at entrance → switch to outdoor flight
                b->bewegung = AUSSENFLUG;
                if (b->auftrag_id >= 0) {
                    b->ziel_x = spiel->auftraege[b->auftrag_id].ziel_x;
                    b->ziel_y = spiel->auftraege[b->auftrag_id].ziel_y;
                }
            } else {
                biene_bewegen(b, spiel, dt);
            }
            break;

        case AUSSENFLUG:
            if (am_ziel(b)) {
                b->bewegung = AM_SAMMELN;
            } else {
                biene_bewegen(b, spiel, dt);
            }
            break;

        case AM_SAMMELN:
            biene_sammeln(b, spiel, dt);
            // Full? Head home
            if (b->honigmagen_mg   >= MAX_HONIGMAGEN_MG * 0.9f
             || b->pollenladung_mg >= MAX_POLLENLADUNG_MG * 0.9f) {
                auftrag_freigeben(b, spiel);
                b->bewegung = HEIMFLUG;
                b->ziel_x   = STOCK_EINGANG_X;
                b->ziel_y   = STOCK_EINGANG_Y;
            }
            break;

        case HEIMFLUG:
            biene_pheromon_abgeben(b,
                &spiel->pheromonfelder[PHEROMON_NASONOV],
                PHEROMON_NASONOV, dt);
            if (am_ziel(b)) {
                // Arrived at entrance → find drop-off inside
                b->bewegung = ZUM_ABLADEN;
                b->ziel_x   = STOCK_LAGER_X;
                b->ziel_y   = STOCK_LAGER_Y;
            } else {
                biene_bewegen(b, spiel, dt);
            }
            break;

        case ZUM_ABLADEN:
            if (am_ziel(b)) {
                biene_ressource_abladen(b, spiel);
                b->bewegung = IM_STOCK_ARBEITEND;
            } else {
                biene_bewegen(b, spiel, dt);
            }
            break;

        case RASTEN:
            // Recover until energy is acceptable again
            if (b->energie_j >= ENERGIE_START_J * 0.4f) {
                b->bewegung = IM_STOCK_ARBEITEND;
            }
            // Eat a tiny bit from colony honey store if available
            if (spiel->honig_reif_g > 0.001f) {
                float essen_g = 0.0001f * dt;
                spiel->honig_reif_g -= essen_g;
                b->energie_j += energie_aufnahme(essen_g * ZUCKERGEHALT_NEKTAR);
                if (b->energie_j > ENERGIE_START_J) b->energie_j = ENERGIE_START_J;
            }
            break;
    }

    // --- PAY COST ---
    biene_energie_verbrauchen(b, leistung_fuer_bewegung(b->bewegung), dt);

    // --- DEATH CHECK ---
    if (biene_ist_tot(b)) {
        auftrag_freigeben(b, spiel);
        b->aktiv = false;
        spiel->volksgroesse--;
        if (spiel->volksgroesse < 0) spiel->volksgroesse = 0;
    }
}

// ---------------------------------------------------------------------------
// alle_bienen_aktualisieren — batch update for the full flat array
// ---------------------------------------------------------------------------
void alle_bienen_aktualisieren(Spielzustand* spiel, float dt) {
    for (int i = 0; i < spiel->bienen_anzahl; i++) {
        biene_aktualisieren(&spiel->bienen[i], spiel, dt);
    }
}
