#include "sim_stock.h"
#include "agent_biene.h"   // biene_init, STOCK_LAGER_X/Y
#include "haushalt.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Count active bees with a given role
static int anzahl_mit_rolle(const Spielzustand* spiel, BienRolle rolle) {
    int n = 0;
    for (int i = 0; i < spiel->bienen_anzahl; i++)
        if (spiel->bienen[i].aktiv && spiel->bienen[i].rolle == rolle) n++;
    return n;
}

// Count unclaimed, active jobs of a given type
static int offene_auftraege(const Spielzustand* spiel, AuftragsTyp typ) {
    int n = 0;
    for (int i = 0; i < spiel->auftraege_anzahl; i++) {
        const Auftrag* a = &spiel->auftraege[i];
        if (a->aktiv && !a->vergeben && a->typ == typ) n++;
    }
    return n;
}

// Add a job — reuses an inactive slot or extends the array
static void auftrag_hinzufuegen(Spielzustand* spiel, AuftragsTyp typ,
                                 int ziel_zelle, float x, float y, int prio) {
    // Scan for a free (inactive) slot to reuse
    for (int i = 0; i < spiel->auftraege_anzahl; i++) {
        if (!spiel->auftraege[i].aktiv) {
            spiel->auftraege[i] = (Auftrag){ typ, ziel_zelle, x, y, prio, false, true };
            return;
        }
    }
    // No free slot: extend array if space available
    if (spiel->auftraege_anzahl < MAX_AUFTRAEGE) {
        spiel->auftraege[spiel->auftraege_anzahl++] =
            (Auftrag){ typ, ziel_zelle, x, y, prio, false, true };
    }
}

// Retire all unclaimed (not vergeben) jobs of a given type (mark slot inactive)
static void auftraege_zurueckziehen(Spielzustand* spiel, AuftragsTyp typ) {
    for (int i = 0; i < spiel->auftraege_anzahl; i++) {
        Auftrag* a = &spiel->auftraege[i];
        if (a->aktiv && !a->vergeben && a->typ == typ)
            a->aktiv = false;
    }
}

// Spawn a new adult bee (PUTZERIN) — reuses dead slot or extends array
static void biene_schluepfen(Spielzustand* spiel) {
    for (int i = 0; i < spiel->bienen_anzahl; i++) {
        if (!spiel->bienen[i].aktiv) {
            biene_init(&spiel->bienen[i], STOCK_LAGER_X, STOCK_LAGER_Y, PUTZERIN);
            spiel->volksgroesse++;
            return;
        }
    }
    if (spiel->bienen_anzahl < MAX_BIENEN) {
        biene_init(&spiel->bienen[spiel->bienen_anzahl],
                   STOCK_LAGER_X, STOCK_LAGER_Y, PUTZERIN);
        spiel->bienen_anzahl++;
        spiel->volksgroesse++;
    }
}

// ---------------------------------------------------------------------------
// stock_init
// ---------------------------------------------------------------------------
void stock_init(Spielzustand* spiel) {
    // Climate
    spiel->temperatur_c    = 35.0f;
    spiel->bedrohungslevel = 0.0f;

    // Starting resources (a young but established colony)
    spiel->honig_reif_g    = 500.0f;   // 500 g ripe honey
    spiel->nektar_unreif_g = 0.0f;
    spiel->pollen_g        = 80.0f;    // 80 g pollen
    spiel->wasser_g        = 15.0f;
    spiel->wachs_mg        = 3000.0f;  // ~2 cells worth of wax

    // Brood counts: tally from the hex grid filled by bienenstock_init (runs before stock_init)
    spiel->brut_ei    = 0;
    spiel->brut_larve = 0;
    spiel->brut_puppe = 0;
    for (int i = 0; i < spiel->zellen_anzahl; i++) {
        switch (spiel->zellen[i].typ) {
            case ZELLE_BRUT_EI:    spiel->brut_ei++;    break;
            case ZELLE_BRUT_LARVE: spiel->brut_larve++; break;
            case ZELLE_BRUT_PUPPE: spiel->brut_puppe++; break;
            default: break;
        }
    }

    // Starting worker bees (10 foragers, distributed across typical age roles)
    spiel->volksgroesse = 0;
    spiel->bienen_anzahl = 0;

    // 3 nurses (age ~8 days), 3 builders (age ~14 days), 4 foragers (age ~26 days)
    float alter_amme    = 8.0f;
    float alter_bauerin = 14.0f;
    float alter_samml   = 26.0f;

    for (int i = 0; i < 3; i++) {
        biene_init(&spiel->bienen[spiel->bienen_anzahl], STOCK_LAGER_X, STOCK_LAGER_Y, AMME);
        spiel->bienen[spiel->bienen_anzahl].alter_t = alter_amme;
        spiel->bienen_anzahl++;
        spiel->volksgroesse++;
    }
    for (int i = 0; i < 3; i++) {
        biene_init(&spiel->bienen[spiel->bienen_anzahl], STOCK_LAGER_X, STOCK_LAGER_Y, BAUERIN);
        spiel->bienen[spiel->bienen_anzahl].alter_t = alter_bauerin;
        spiel->bienen_anzahl++;
        spiel->volksgroesse++;
    }
    for (int i = 0; i < 4; i++) {
        biene_init(&spiel->bienen[spiel->bienen_anzahl], ZONE_EINGANG_X, ZONE_EINGANG_Y,
                   SAMMLERIN_NEKTAR);
        spiel->bienen[spiel->bienen_anzahl].alter_t = alter_samml;
        spiel->bienen_anzahl++;
        spiel->volksgroesse++;
    }

    // Job queue starts empty; stock_aktualisieren fills it on first tick
    spiel->auftraege_anzahl = 0;
}

// ---------------------------------------------------------------------------
// stock_brut_aktualisieren
// ---------------------------------------------------------------------------
void stock_brut_aktualisieren(Spielzustand* spiel, float dt) {
    float tage_dt = dt / 86400.0f;   // Real seconds → simulated days

    spiel->brut_ei    = 0;
    spiel->brut_larve = 0;
    spiel->brut_puppe = 0;

    for (int i = 0; i < spiel->zellen_anzahl; i++) {
        Zelle* z = &spiel->zellen[i];

        switch (z->typ) {
            case ZELLE_BRUT_EI:
                z->alter_t += tage_dt;
                if (z->alter_t >= EI_DAUER_T) {
                    z->typ    = ZELLE_BRUT_LARVE;
                    z->alter_t = 0.0f;
                } else {
                    spiel->brut_ei++;
                }
                break;

            case ZELLE_BRUT_LARVE:
                z->alter_t += tage_dt;
                if (z->alter_t >= LARVEN_DAUER_T) {
                    z->typ    = ZELLE_BRUT_PUPPE;
                    z->alter_t = 0.0f;
                } else {
                    spiel->brut_larve++;
                    // Larva may die from starvation or thermal stress
                    float hunger = (spiel->pollen_g < 5.0f) ? 0.8f : 0.0f;
                    float p_tod  = brut_sterblichkeit(spiel->temperatur_c, hunger);
                    if (p_tod > 0.0f && (z->alter_t * 137.0f - (int)(z->alter_t * 137.0f)) < p_tod * tage_dt) {
                        z->typ    = ZELLE_LEER;
                        z->sauber = false;
                        spiel->brut_larve--;
                    }
                }
                break;

            case ZELLE_BRUT_PUPPE:
                z->alter_t += tage_dt;
                if (z->alter_t >= PUPPEN_DAUER_T) {
                    z->typ     = ZELLE_LEER;
                    z->alter_t = 0.0f;
                    z->sauber  = false;   // Cell needs cleaning before reuse
                    biene_schluepfen(spiel);
                } else {
                    spiel->brut_puppe++;
                    // Thermal stress can kill pupae (less tolerant than larvae)
                    float p_tod = brut_sterblichkeit(spiel->temperatur_c, 0.0f);
                    if (p_tod > 0.0f && (z->alter_t * 173.0f - (int)(z->alter_t * 173.0f)) < p_tod * tage_dt) {
                        z->typ    = ZELLE_LEER;
                        z->sauber = false;
                        spiel->brut_puppe--;
                    }
                }
                break;

            default:
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// stock_auftraege_generieren
// ---------------------------------------------------------------------------
void stock_auftraege_generieren(Spielzustand* spiel) {

    // --- Priority 1: Feed larvae ---
    {
        int bedarf  = spiel->brut_larve;
        int bereits = offene_auftraege(spiel, AUFTRAG_LARVE_FUETTERN);
        int anzahl  = bedarf - bereits;
        for (int i = 0; i < anzahl; i++)
            auftrag_hinzufuegen(spiel, AUFTRAG_LARVE_FUETTERN, -1,
                                ZONE_BRUT_X, ZONE_BRUT_Y, 1);
        // Retire excess if brood count dropped
        if (bereits > bedarf)
            auftraege_zurueckziehen(spiel, AUFTRAG_LARVE_FUETTERN);
    }

    // --- Priority 1: Guard entrance (always at least 1) ---
    if (offene_auftraege(spiel, AUFTRAG_EINGANG_BEWACHEN) == 0)
        auftrag_hinzufuegen(spiel, AUFTRAG_EINGANG_BEWACHEN, -1,
                            ZONE_EINGANG_X, ZONE_EINGANG_Y, 1);

    // --- Priority 2: Thermal regulation (fan if too hot) ---
    if (spiel->temperatur_c > 36.0f) {
        int faecher_bedarf = (int)((spiel->temperatur_c - 35.0f) * 4.0f) + 1;
        int bereits        = offene_auftraege(spiel, AUFTRAG_FAECHELN);
        int anzahl         = faecher_bedarf - bereits;
        for (int i = 0; i < anzahl; i++)
            auftrag_hinzufuegen(spiel, AUFTRAG_FAECHELN, -1,
                                ZONE_EINGANG_X, ZONE_EINGANG_Y, 2);
    } else {
        // Too cool or optimal — retire unclaimed fanning jobs
        auftraege_zurueckziehen(spiel, AUFTRAG_FAECHELN);
    }

    // --- Priority 2: Process nectar into honey ---
    if (spiel->nektar_unreif_g > 0.1f) {
        int bedarf  = (int)(spiel->nektar_unreif_g / 5.0f) + 1;
        int bereits = offene_auftraege(spiel, AUFTRAG_NEKTAR_VERARBEITEN);
        int anzahl  = bedarf - bereits;
        for (int i = 0; i < anzahl; i++)
            auftrag_hinzufuegen(spiel, AUFTRAG_NEKTAR_VERARBEITEN, -1,
                                ZONE_HONIG_X, ZONE_HONIG_Y, 2);
    }

    // --- Priority 2: Clean dirty cells ---
    {
        int dirty = 0;
        for (int i = 0; i < spiel->zellen_anzahl; i++)
            if (spiel->zellen[i].typ == ZELLE_LEER && !spiel->zellen[i].sauber) dirty++;
        int bereits = offene_auftraege(spiel, AUFTRAG_ZELLE_REINIGEN);
        int anzahl  = dirty - bereits;
        for (int i = 0; i < anzahl; i++)
            auftrag_hinzufuegen(spiel, AUFTRAG_ZELLE_REINIGEN, -1,
                                ZONE_BRUT_X, ZONE_BRUT_Y, 2);
    }

    // --- Priority 3: Forage nectar (30% of colony) ---
    {
        int bedarf  = spiel->volksgroesse / 3;
        if (bedarf < 1) bedarf = 1;
        int bereits = offene_auftraege(spiel, AUFTRAG_NEKTAR_SAMMELN);
        int anzahl  = bedarf - bereits;
        for (int i = 0; i < anzahl; i++) {
            // Pick first active nectar source
            for (int q = 0; q < spiel->quellen_anzahl; q++) {
                Trachtquelle* src = &spiel->quellen[q];
                if (src->aktiv && src->typ == TRACHT_NEKTAR && src->vorrat_g > 0.0f) {
                    auftrag_hinzufuegen(spiel, AUFTRAG_NEKTAR_SAMMELN, -1,
                                        src->pos_x, src->pos_y, 3);
                    break;
                }
            }
        }
    }

    // --- Priority 3: Forage pollen if reserves low ---
    if (spiel->pollen_g < 100.0f) {
        int bereits = offene_auftraege(spiel, AUFTRAG_POLLEN_SAMMELN);
        int anzahl  = 3 - bereits;
        for (int i = 0; i < anzahl; i++) {
            for (int q = 0; q < spiel->quellen_anzahl; q++) {
                Trachtquelle* src = &spiel->quellen[q];
                if (src->aktiv && src->typ == TRACHT_POLLEN && src->vorrat_g > 0.0f) {
                    auftrag_hinzufuegen(spiel, AUFTRAG_POLLEN_SAMMELN, -1,
                                        src->pos_x, src->pos_y, 3);
                    break;
                }
            }
        }
    }

    // --- Priority 3: Build comb if wax available and space needed ---
    if (spiel->wachs_mg >= WACHS_PRO_ZELLE_MG) {
        if (offene_auftraege(spiel, AUFTRAG_WABE_BAUEN) == 0)
            auftrag_hinzufuegen(spiel, AUFTRAG_WABE_BAUEN, -1,
                                ZONE_WABENBAU_X, ZONE_WABENBAU_Y, 3);
    }
}

// ---------------------------------------------------------------------------
// stock_nektar_verarbeiten
// ---------------------------------------------------------------------------
void stock_nektar_verarbeiten(Spielzustand* spiel, float dt) {
    if (spiel->nektar_unreif_g <= 0.0f) return;

    // Evaporation driven by fanning and nectar-processing bees
    int faecher = anzahl_mit_rolle(spiel, FAECHER)
                + anzahl_mit_rolle(spiel, EMPFAENGERIN);

    float verdunstung = nektar_verdunstung(faecher, dt);
    // Can't remove more water than the nectar's water fraction
    float max_verdunstung = spiel->nektar_unreif_g * (1.0f - ZUCKERGEHALT_NEKTAR);
    if (verdunstung > max_verdunstung) verdunstung = max_verdunstung;
    spiel->nektar_unreif_g -= verdunstung;

    // Convert a batch of concentrated nectar to ripe honey each tick
    float rate        = (faecher + 1) * FAECHER_NEKTAR_RATE_G_S;
    float konvertiert = rate * dt;
    if (konvertiert > spiel->nektar_unreif_g) konvertiert = spiel->nektar_unreif_g;

    spiel->honig_reif_g    += honig_aus_nektar(konvertiert, ZUCKERGEHALT_NEKTAR);
    spiel->nektar_unreif_g -= konvertiert;

    // Keep legacy HUD field in sync (0.0–1.0 relative to a 1 kg benchmark)
    spiel->honig = spiel->honig_reif_g / 1000.0f;
    if (spiel->honig > 1.0f) spiel->honig = 1.0f;
}

// ---------------------------------------------------------------------------
// stock_thermoregulieren
// ---------------------------------------------------------------------------
void stock_thermoregulieren(Spielzustand* spiel, float dt) {
    int faecher = anzahl_mit_rolle(spiel, FAECHER);

    // Metabolic heat from all active bees
    float bienen_waerme   = spiel->volksgroesse * 0.003f;

    // Passive cooling toward ambient (20°C outside)
    float ambient_verlust = (spiel->temperatur_c - 20.0f) * 0.004f;

    // Active cooling from fanning bees
    float faecheln_kuehl  = faecher * 0.05f;

    float delta = (bienen_waerme - ambient_verlust - faecheln_kuehl) * dt;
    spiel->temperatur_c += delta;

    if (spiel->temperatur_c < 15.0f) spiel->temperatur_c = 15.0f;
    if (spiel->temperatur_c > 45.0f) spiel->temperatur_c = 45.0f;
}

// ---------------------------------------------------------------------------
// stock_wabe_bauen
// ---------------------------------------------------------------------------
void stock_wabe_bauen(Spielzustand* spiel, float dt) {
    int bauerinnen = anzahl_mit_rolle(spiel, BAUERIN);
    if (bauerinnen <= 0) return;

    // Bees consume honey to produce wax glands secretion
    float honig_verbraucht = bauerinnen * BIENE_WACHSPROD_G_S * dt;
    if (honig_verbraucht > spiel->honig_reif_g)
        honig_verbraucht = spiel->honig_reif_g;
    spiel->honig_reif_g -= honig_verbraucht;

    // Convert sugar to wax (honey is ~82% sugar by mass)
    float wachs_neu = wachs_aus_zucker(honig_verbraucht * (1.0f - WASSERANTEIL_HONIG));
    spiel->wachs_mg += wachs_neu;

    // When enough wax accumulates, add a new empty cell to the simulation grid
    while (spiel->wachs_mg >= WACHS_PRO_ZELLE_MG && spiel->zellen_anzahl < MAX_ZELLEN) {
        spiel->wachs_mg -= WACHS_PRO_ZELLE_MG;
        Zelle* z  = &spiel->zellen[spiel->zellen_anzahl++];
        z->typ    = ZELLE_LEER;
        z->inhalt  = 0.0f;
        z->alter_t = 0.0f;
        z->sauber  = true;
    }
}

// ---------------------------------------------------------------------------
// stock_aktualisieren — main colony tick
// ---------------------------------------------------------------------------
void stock_aktualisieren(Spielzustand* spiel, float dt) {
    stock_brut_aktualisieren(spiel, dt);
    stock_thermoregulieren(spiel, dt);
    stock_nektar_verarbeiten(spiel, dt);
    stock_wabe_bauen(spiel, dt);
    stock_auftraege_generieren(spiel);
}
