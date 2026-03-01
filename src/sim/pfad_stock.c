#include "pfad_stock.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Direction vector stored per cell: signed step (-1, 0, +1) on each axis
// ---------------------------------------------------------------------------
typedef struct { int8_t dx; int8_t dy; } Richtung;

// ---------------------------------------------------------------------------
// Static storage — one flowfield per zone
// ---------------------------------------------------------------------------
static Richtung felder[STOCK_ZONE_ANZAHL][PFAD_FF_ZELLEN];
static bool     gueltig[STOCK_ZONE_ANZAHL];

// ---------------------------------------------------------------------------
// Zone center coordinates (screen pixels in ANSICHT_STOCK)
// These match the zone constants in sim_stock.h
// ---------------------------------------------------------------------------
static const float zone_x[STOCK_ZONE_ANZAHL] = {
    180.0f,   // STOCK_ZONE_EINGANG
    640.0f,   // STOCK_ZONE_LAGER
    400.0f,   // STOCK_ZONE_BRUT
    680.0f,   // STOCK_ZONE_HONIG
    680.0f,   // STOCK_ZONE_POLLEN
};
static const float zone_y[STOCK_ZONE_ANZAHL] = {
    370.0f,
    360.0f,
    360.0f,
    250.0f,
    470.0f,
};

// ---------------------------------------------------------------------------
// BFS — compute cost-to-target grid, then derive directions
// All indoor cells are passable (no obstacles yet; ready for Phase 9 additions).
// ---------------------------------------------------------------------------
static void bfs_berechnen(Richtung* feld, int ziel_gx, int ziel_gy) {
    static uint16_t kosten[PFAD_FF_ZELLEN];
    static int      queue[PFAD_FF_ZELLEN];

    // Reset costs to "unreachable"
    memset(kosten, 0xFF, sizeof(kosten));

    int ziel_idx  = ziel_gy * PFAD_FF_BREITE + ziel_gx;
    kosten[ziel_idx] = 0;

    int head = 0, tail = 0;
    queue[tail++] = ziel_idx;

    // 8-directional BFS from target outward
    while (head != tail) {
        int  curr = queue[head++];
        int  cx   = curr % PFAD_FF_BREITE;
        int  cy   = curr / PFAD_FF_BREITE;
        uint16_t next_kosten = (uint16_t)(kosten[curr] + 1);

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx;
                int ny = cy + dy;
                if (nx < 0 || nx >= PFAD_FF_BREITE) continue;
                if (ny < 0 || ny >= PFAD_FF_HOEHE)  continue;
                int ni = ny * PFAD_FF_BREITE + nx;
                if (kosten[ni] <= next_kosten)       continue;
                kosten[ni] = next_kosten;
                queue[tail++] = ni;
            }
        }
    }

    // Build direction field: each cell points at its lowest-cost neighbour
    for (int i = 0; i < PFAD_FF_ZELLEN; i++) {
        if (kosten[i] == 0xFFFF) { feld[i] = (Richtung){0, 0}; continue; }

        int cx = i % PFAD_FF_BREITE;
        int cy = i / PFAD_FF_BREITE;
        int8_t best_dx = 0, best_dy = 0;
        uint16_t best = kosten[i];

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

// ---------------------------------------------------------------------------
// pfad_stock_berechnen
// ---------------------------------------------------------------------------
void pfad_stock_berechnen(StockZone zone) {
    int gx = (int)(zone_x[zone] / PFAD_ZELLEN_GROESSE);
    int gy = (int)(zone_y[zone] / PFAD_ZELLEN_GROESSE);

    // Clamp to grid
    if (gx < 0) gx = 0; if (gx >= PFAD_FF_BREITE) gx = PFAD_FF_BREITE - 1;
    if (gy < 0) gy = 0; if (gy >= PFAD_FF_HOEHE)  gy = PFAD_FF_HOEHE  - 1;

    bfs_berechnen(felder[zone], gx, gy);
    gueltig[zone] = true;
}

// ---------------------------------------------------------------------------
// pfad_stock_init — compute all zone flowfields at startup
// ---------------------------------------------------------------------------
void pfad_stock_init(void) {
    for (int z = 0; z < STOCK_ZONE_ANZAHL; z++)
        pfad_stock_berechnen((StockZone)z);
}

// ---------------------------------------------------------------------------
// pfad_stock_richtung — query direction for a bee at screen position (x,y)
// ---------------------------------------------------------------------------
Vector2 pfad_stock_richtung(float pos_x, float pos_y, StockZone zone) {
    if (!gueltig[zone]) return (Vector2){0, 0};

    int gx = (int)(pos_x / PFAD_ZELLEN_GROESSE);
    int gy = (int)(pos_y / PFAD_ZELLEN_GROESSE);
    if (gx < 0 || gx >= PFAD_FF_BREITE) return (Vector2){0, 0};
    if (gy < 0 || gy >= PFAD_FF_HOEHE)  return (Vector2){0, 0};

    Richtung r = felder[zone][gy * PFAD_FF_BREITE + gx];
    if (r.dx == 0 && r.dy == 0) return (Vector2){0, 0};

    // Normalize so diagonal steps have the same speed as cardinal steps
    float len = (r.dx != 0 && r.dy != 0) ? 1.41421356f : 1.0f;
    return (Vector2){ (float)r.dx / len, (float)r.dy / len };
}
