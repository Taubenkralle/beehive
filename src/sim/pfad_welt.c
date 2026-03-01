#include "pfad_welt.h"
#include "agent_biene.h"   // STOCK_EINGANG_X / STOCK_EINGANG_Y
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Direction vector stored per cell
// ---------------------------------------------------------------------------
typedef struct { int8_t dx; int8_t dy; } Richtung;

// ---------------------------------------------------------------------------
// Storage — one outdoor flowfield per forage source + one for hive entrance
// Uses the same grid dimensions as pfad_stock (PFAD_FF_BREITE × PFAD_FF_HOEHE)
// ---------------------------------------------------------------------------
static Richtung quellen_felder[MAX_QUELLEN][PFAD_FF_ZELLEN];
static bool     quellen_gueltig[MAX_QUELLEN];

static Richtung eingang_feld[PFAD_FF_ZELLEN];
static bool     eingang_gueltig = false;

// ---------------------------------------------------------------------------
// BFS (same algorithm as pfad_stock, works for any target cell)
// ---------------------------------------------------------------------------
static void bfs_berechnen(Richtung* feld, int ziel_gx, int ziel_gy) {
    static uint16_t kosten[PFAD_FF_ZELLEN];
    static int      queue[PFAD_FF_ZELLEN];

    memset(kosten, 0xFF, sizeof(kosten));

    int ziel_idx      = ziel_gy * PFAD_FF_BREITE + ziel_gx;
    kosten[ziel_idx]  = 0;

    int head = 0, tail = 0;
    queue[tail++] = ziel_idx;

    while (head != tail) {
        int curr        = queue[head++];
        int cx          = curr % PFAD_FF_BREITE;
        int cy          = curr / PFAD_FF_BREITE;
        uint16_t nkosten = (uint16_t)(kosten[curr] + 1);

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx, ny = cy + dy;
                if (nx < 0 || nx >= PFAD_FF_BREITE) continue;
                if (ny < 0 || ny >= PFAD_FF_HOEHE)  continue;
                int ni = ny * PFAD_FF_BREITE + nx;
                if (kosten[ni] <= nkosten) continue;
                kosten[ni] = nkosten;
                queue[tail++] = ni;
            }
        }
    }

    for (int i = 0; i < PFAD_FF_ZELLEN; i++) {
        if (kosten[i] == 0xFFFF) { feld[i] = (Richtung){0, 0}; continue; }

        int cx = i % PFAD_FF_BREITE;
        int cy = i / PFAD_FF_BREITE;
        int8_t  best_dx = 0, best_dy = 0;
        uint16_t best   = kosten[i];

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx, ny = cy + dy;
                if (nx < 0 || nx >= PFAD_FF_BREITE) continue;
                if (ny < 0 || ny >= PFAD_FF_HOEHE)  continue;
                int ni = ny * PFAD_FF_BREITE + nx;
                if (kosten[ni] < best) {
                    best    = kosten[ni];
                    best_dx = (int8_t)dx;
                    best_dy = (int8_t)dy;
                }
            }
        }
        feld[i] = (Richtung){best_dx, best_dy};
    }
}

// Helper: world position → clamped grid cell
static int pos_to_gx(float x) {
    int gx = (int)(x / PFAD_ZELLEN_GROESSE);
    if (gx < 0) gx = 0;
    if (gx >= PFAD_FF_BREITE) gx = PFAD_FF_BREITE - 1;
    return gx;
}
static int pos_to_gy(float y) {
    int gy = (int)(y / PFAD_ZELLEN_GROESSE);
    if (gy < 0) gy = 0;
    if (gy >= PFAD_FF_HOEHE) gy = PFAD_FF_HOEHE - 1;
    return gy;
}

// Helper: query a direction field at screen position (x,y)
static Vector2 feld_richtung(const Richtung* feld, float pos_x, float pos_y) {
    int gx = (int)(pos_x / PFAD_ZELLEN_GROESSE);
    int gy = (int)(pos_y / PFAD_ZELLEN_GROESSE);
    if (gx < 0 || gx >= PFAD_FF_BREITE) return (Vector2){0, 0};
    if (gy < 0 || gy >= PFAD_FF_HOEHE)  return (Vector2){0, 0};

    Richtung r = feld[gy * PFAD_FF_BREITE + gx];
    if (r.dx == 0 && r.dy == 0) return (Vector2){0, 0};

    float len = (r.dx != 0 && r.dy != 0) ? 1.41421356f : 1.0f;
    return (Vector2){ (float)r.dx / len, (float)r.dy / len };
}

// ---------------------------------------------------------------------------
// pfad_welt_berechnen — flowfield toward one forage source
// ---------------------------------------------------------------------------
void pfad_welt_berechnen(int quellen_id, const Spielzustand* spiel) {
    if (quellen_id < 0 || quellen_id >= MAX_QUELLEN) return;
    const Trachtquelle* q = &spiel->quellen[quellen_id];
    bfs_berechnen(quellen_felder[quellen_id],
                  pos_to_gx(q->pos_x), pos_to_gy(q->pos_y));
    quellen_gueltig[quellen_id] = true;
}

// ---------------------------------------------------------------------------
// pfad_eingang_berechnen — flowfield leading back to the hive entrance
// ---------------------------------------------------------------------------
void pfad_eingang_berechnen(void) {
    bfs_berechnen(eingang_feld,
                  pos_to_gx(STOCK_EINGANG_X), pos_to_gy(STOCK_EINGANG_Y));
    eingang_gueltig = true;
}

// ---------------------------------------------------------------------------
// pfad_welt_init — compute all flowfields at startup
// ---------------------------------------------------------------------------
void pfad_welt_init(const Spielzustand* spiel) {
    memset(quellen_gueltig, 0, sizeof(quellen_gueltig));
    eingang_gueltig = false;

    for (int i = 0; i < spiel->quellen_anzahl; i++)
        pfad_welt_berechnen(i, spiel);

    pfad_eingang_berechnen();
}

// ---------------------------------------------------------------------------
// Query functions
// ---------------------------------------------------------------------------
Vector2 pfad_welt_richtung(float pos_x, float pos_y, int quellen_id) {
    if (quellen_id < 0 || quellen_id >= MAX_QUELLEN) return (Vector2){0, 0};
    if (!quellen_gueltig[quellen_id])                return (Vector2){0, 0};
    return feld_richtung(quellen_felder[quellen_id], pos_x, pos_y);
}

Vector2 pfad_eingang_richtung(float pos_x, float pos_y) {
    if (!eingang_gueltig) return (Vector2){0, 0};
    return feld_richtung(eingang_feld, pos_x, pos_y);
}
