#include "pheromon.h"
#include "../kern/spielzustand.h"  // For FENSTER_BREITE, FENSTER_HOEHE, raylib
#include <string.h>

// ---------------------------------------------------------------------------
// Coordinate helpers: world pixel → grid cell
// ---------------------------------------------------------------------------
static inline int gx_von_welt(float x) {
    int gx = (int)(x * PHEROMON_BREITE / FENSTER_BREITE);
    if (gx < 0)              gx = 0;
    if (gx >= PHEROMON_BREITE) gx = PHEROMON_BREITE - 1;
    return gx;
}

static inline int gy_von_welt(float y) {
    int gy = (int)(y * PHEROMON_HOEHE / FENSTER_HOEHE);
    if (gy < 0)             gy = 0;
    if (gy >= PHEROMON_HOEHE) gy = PHEROMON_HOEHE - 1;
    return gy;
}

static inline int gitter_idx(int gx, int gy) {
    return gy * PHEROMON_BREITE + gx;
}

// ---------------------------------------------------------------------------
// Init: zero all fields, set tuned physical constants
// ---------------------------------------------------------------------------
void pheromon_init(PheromonFeld felder[PHEROMON_ANZAHL]) {
    memset(felder, 0, sizeof(PheromonFeld) * PHEROMON_ANZAHL);

    // Spurpheromon: spreads slowly, lingers long (marks routes)
    felder[PHEROMON_SPUR].diffusion = 0.8f;
    felder[PHEROMON_SPUR].zerfall   = 0.04f;

    // Alarmpheromon: spreads fast, fades quickly (urgent signal)
    felder[PHEROMON_ALARM].diffusion = 3.0f;
    felder[PHEROMON_ALARM].zerfall   = 0.35f;

    // Nasonov: medium spread, medium lifetime (home beacon)
    felder[PHEROMON_NASONOV].diffusion = 1.5f;
    felder[PHEROMON_NASONOV].zerfall   = 0.10f;
}

// ---------------------------------------------------------------------------
// Aktualisieren: diffusion + decay, one tick per frame
//
// Formula per cell: C_neu = C + D * Laplace(C) * dt - k * C * dt
// Laplace (discrete): C[x-1,y] + C[x+1,y] + C[x,y-1] + C[x,y+1] - 4*C[x,y]
//
// In-place update — slightly inaccurate at cell boundaries but visually
// correct and avoids a double-buffer allocation for 27KB of grid data.
// ---------------------------------------------------------------------------
void pheromon_aktualisieren(PheromonFeld felder[PHEROMON_ANZAHL], float dt) {
    for (int f = 0; f < PHEROMON_ANZAHL; f++) {
        PheromonFeld* p = &felder[f];
        float* z = p->zellen;
        float  D = p->diffusion;
        float  k = p->zerfall;

        // Skip inner border rows/cols — avoids boundary index checks in hot loop
        for (int gy = 1; gy < PHEROMON_HOEHE - 1; gy++) {
            for (int gx = 1; gx < PHEROMON_BREITE - 1; gx++) {
                int i = gitter_idx(gx, gy);
                float laplace = z[gitter_idx(gx-1, gy)]
                              + z[gitter_idx(gx+1, gy)]
                              + z[gitter_idx(gx, gy-1)]
                              + z[gitter_idx(gx, gy+1)]
                              - 4.0f * z[i];

                z[i] += (D * laplace - k * z[i]) * dt;
                if (z[i] < 0.0f) z[i] = 0.0f;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Abgeben: bee deposits pheromone at world position
// ---------------------------------------------------------------------------
void pheromon_abgeben(PheromonFeld* feld, float welt_x, float welt_y, float menge) {
    int gx = gx_von_welt(welt_x);
    int gy = gy_von_welt(welt_y);
    int i  = gitter_idx(gx, gy);

    feld->zellen[i] += menge;
    if (feld->zellen[i] > 100.0f) feld->zellen[i] = 100.0f;  // Cap to prevent runaway
}

// ---------------------------------------------------------------------------
// Lesen: bilinear interpolation for smooth gradient following
// ---------------------------------------------------------------------------
float pheromon_lesen(const PheromonFeld* feld, float welt_x, float welt_y) {
    float fx = welt_x * PHEROMON_BREITE / FENSTER_BREITE;
    float fy = welt_y * PHEROMON_HOEHE  / FENSTER_HOEHE;

    int gx0 = (int)fx;  int gx1 = gx0 + 1;
    int gy0 = (int)fy;  int gy1 = gy0 + 1;

    // Clamp to valid range
    if (gx0 < 0) gx0 = 0;  if (gx0 >= PHEROMON_BREITE) gx0 = PHEROMON_BREITE - 1;
    if (gx1 < 0) gx1 = 0;  if (gx1 >= PHEROMON_BREITE) gx1 = PHEROMON_BREITE - 1;
    if (gy0 < 0) gy0 = 0;  if (gy0 >= PHEROMON_HOEHE)  gy0 = PHEROMON_HOEHE  - 1;
    if (gy1 < 0) gy1 = 0;  if (gy1 >= PHEROMON_HOEHE)  gy1 = PHEROMON_HOEHE  - 1;

    float tx = fx - (float)(int)fx;
    float ty = fy - (float)(int)fy;

    float c00 = feld->zellen[gitter_idx(gx0, gy0)];
    float c10 = feld->zellen[gitter_idx(gx1, gy0)];
    float c01 = feld->zellen[gitter_idx(gx0, gy1)];
    float c11 = feld->zellen[gitter_idx(gx1, gy1)];

    return c00*(1.0f-tx)*(1.0f-ty) + c10*tx*(1.0f-ty)
         + c01*(1.0f-tx)*ty        + c11*tx*ty;
}

// ---------------------------------------------------------------------------
// Zeichnen: debug heatmap overlay — call after scene draw, before EndDrawing
// Colors: Spur=grün, Alarm=rot, Nasonov=blau
// ---------------------------------------------------------------------------
void pheromon_zeichnen(const PheromonFeld* feld, PheromonTyp typ) {
    float px_b = (float)FENSTER_BREITE / PHEROMON_BREITE;
    float px_h = (float)FENSTER_HOEHE  / PHEROMON_HOEHE;

    unsigned char r = 0, g = 0, b = 0;
    switch (typ) {
        case PHEROMON_SPUR:    g = 220; b = 80;  break;  // Gruen
        case PHEROMON_ALARM:   r = 255; g = 60;  break;  // Rot
        case PHEROMON_NASONOV: b = 255; g = 120; break;  // Blau
    }

    for (int gy = 0; gy < PHEROMON_HOEHE; gy++) {
        for (int gx = 0; gx < PHEROMON_BREITE; gx++) {
            float konz = feld->zellen[gitter_idx(gx, gy)];
            if (konz < 0.3f) continue;  // Skip near-zero for performance

            unsigned char alpha = (unsigned char)(konz * 2.2f);
            if (alpha > 170) alpha = 170;

            DrawRectangle(
                (int)(gx * px_b), (int)(gy * px_h),
                (int)px_b + 1,    (int)px_h + 1,
                (Color){r, g, b, alpha}
            );
        }
    }
}
