#include "bienenstock.h"
#include <math.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Hex-Gitter Layout  (flache Hexagone, Rotation=0 in DrawPoly)
//   Breite Spitze-zu-Spitze  = 2*R
//   Hoehe  Flaeche-zu-Flaeche = sqrt(3)*R
//   Spaltenschritt (x)        = 1.5*R
//   Zeilenschritt  (y)        = sqrt(3)*R
//   Ungerade Spalten: nach unten versetzt um sqrt(3)/2*R
// ---------------------------------------------------------------------------
#define HEX_RADIUS      18.0f
#define HEX_SPALTEN     34
#define HEX_ZEILEN      17
#define HEX_SCHRITT_X   (HEX_RADIUS * 1.5f)
#define HEX_SCHRITT_Y   (HEX_RADIUS * 1.732f)
#define HEX_VERSATZ     (HEX_RADIUS * 0.866f)
#define GITTER_START_X  162.0f
#define GITTER_START_Y  72.0f

// Queen pixel position (visual-only; no Biene entry for the queen yet)
static Vector2 koenigin_pos;

// ---------------------------------------------------------------------------
// Hilfsfunktion: Gitterkoordinate → Bildschirmposition
// ---------------------------------------------------------------------------
static Vector2 hex_bildschirm(int spalte, int zeile) {
    float x = GITTER_START_X + spalte * HEX_SCHRITT_X;
    float y = GITTER_START_Y + zeile  * HEX_SCHRITT_Y
                             + (spalte % 2 == 1 ? HEX_VERSATZ : 0.0f);
    return (Vector2){x, y};
}

// ---------------------------------------------------------------------------
// Zellentyp anhand der Position im Gitter bestimmen
// ---------------------------------------------------------------------------
static ZellenTyp berechne_zellentyp(int spalte, int zeile) {
    float mx      = HEX_SPALTEN / 2.0f;
    float my      = HEX_ZEILEN  / 2.0f;
    float abstand = sqrtf((spalte - mx) * (spalte - mx) + (zeile - my) * (zeile - my));
    float norm    = abstand / (mx < my ? mx : my);  // 0 = Mitte, ~1 = Rand

    // Deterministisches Rauschen fuer lokale Abwechslung
    int z = ((spalte * 1327 + zeile * 4711) ^ (spalte * zeile * 31)) & 0xFF;
    z = z % 100;

    if      (norm < 0.18f) return (z < 65) ? ZELLE_BRUT_LARVE  : ZELLE_BRUT_EI;
    else if (norm < 0.50f) return (z < 48) ? ZELLE_BRUT_LARVE  : (z < 72) ? ZELLE_POLLEN : ZELLE_LEER;
    else if (norm < 0.80f) return (z < 58) ? ZELLE_HONIG_OFFEN : (z < 82) ? ZELLE_POLLEN : ZELLE_LEER;
    else                   return (z < 28) ? ZELLE_HONIG_OFFEN : ZELLE_LEER;
}

static Color zellenfarbe(ZellenTyp typ) {
    switch (typ) {
        case ZELLE_HONIG_OFFEN: return (Color){210, 140,  18, 255};
        case ZELLE_HONIG_VERD:  return (Color){232, 200,  74, 255};
        case ZELLE_POLLEN:      return (Color){215, 185,  40, 255};
        case ZELLE_BRUT_LARVE:  return (Color){195, 160, 115, 255};
        case ZELLE_BRUT_EI:     return (Color){240, 230, 200, 255};
        case ZELLE_BRUT_PUPPE:  return (Color){200, 165,  80, 255};
        case ZELLE_WACHSBAU:    return (Color){237, 216, 112, 255};
        case ZELLE_KOENIGIN:    return (Color){170,  70, 200, 255};
        default:                return (Color){ 30,  16,   4, 255};
    }
}

// ---------------------------------------------------------------------------
// Eine Biene im Stock zeichnen
// ---------------------------------------------------------------------------
static void zeichne_stockbiene(Vector2 pos, bool ist_koenigin, float animzeit) {
    float flattern = sinf(animzeit * 22.0f) * 2.5f;

    // Fluegel
    DrawCircleV((Vector2){pos.x - 7, pos.y - 5 + flattern}, 5, (Color){220, 240, 255, 110});
    DrawCircleV((Vector2){pos.x + 7, pos.y - 5 - flattern}, 5, (Color){220, 240, 255, 110});

    if (ist_koenigin) {
        // Koenigin: groesser, lila-amber
        DrawEllipse((int)pos.x, (int)pos.y, 13, 7, (Color){170,  70, 200, 255});
        DrawEllipse((int)pos.x, (int)pos.y,  4, 7, (Color){ 60,  20,  80, 200});
        DrawCircle((int)pos.x + 11, (int)pos.y - 1, 2, (Color){30, 10, 50, 255});
    } else {
        // Arbeiterin: gelb-schwarz gestreift
        DrawEllipse((int)pos.x,     (int)pos.y, 9, 5, (Color){218, 172,   0, 255});
        DrawEllipse((int)pos.x - 1, (int)pos.y, 3, 5, (Color){ 55,  32,   0, 200});
        DrawEllipse((int)pos.x + 4, (int)pos.y, 2, 5, (Color){ 55,  32,   0, 200});
        DrawCircle((int)pos.x + 8, (int)pos.y - 1, 1, (Color){20, 10, 0, 255});
    }
}

// ---------------------------------------------------------------------------
// Initialisierung
// ---------------------------------------------------------------------------
void bienenstock_init(Spielzustand* spiel) {
    // Populate simulation cell grid from hex layout (row-major: index = zeile*SPALTEN+spalte)
    // This is the canonical source of truth; bienenstock_zeichnen reads it back.
    spiel->zellen_anzahl = 0;
    for (int z = 0; z < HEX_ZEILEN; z++) {
        for (int s = 0; s < HEX_SPALTEN; s++) {
            ZellenTyp typ = berechne_zellentyp(s, z);
            Zelle* zelle  = &spiel->zellen[spiel->zellen_anzahl++];
            zelle->typ    = typ;
            zelle->inhalt = (typ == ZELLE_LEER) ? 0.0f : 1.0f;
            zelle->sauber = (typ == ZELLE_LEER);
            // Stagger initial brood ages so development is not synchronised
            switch (typ) {
                case ZELLE_BRUT_EI:    zelle->alter_t = (float)(rand() % 30)  * 0.1f; break;
                case ZELLE_BRUT_LARVE: zelle->alter_t = (float)(rand() % 60)  * 0.1f; break;
                case ZELLE_BRUT_PUPPE: zelle->alter_t = (float)(rand() % 120) * 0.1f; break;
                default:               zelle->alter_t = 0.0f; break;
            }
        }
    }

    // Queen visual position (center of the grid)
    koenigin_pos = hex_bildschirm(HEX_SPALTEN / 2, HEX_ZEILEN / 2);
}

// ---------------------------------------------------------------------------
// Aktualisierung pro Frame
// ---------------------------------------------------------------------------
void bienenstock_aktualisieren(Spielzustand* spiel, float delta) {
    // zeit is managed by welt_aktualisieren; sim bees by alle_bienen_aktualisieren
    (void)spiel; (void)delta;
}

// ---------------------------------------------------------------------------
// Zeichnen der Querschnitt-Ansicht
// ---------------------------------------------------------------------------
void bienenstock_zeichnen(const Spielzustand* spiel) {
    ClearBackground((Color){22, 11, 3, 255});
    DrawRectangle(0, 0, FENSTER_BREITE, FENSTER_HOEHE, (Color){44, 24, 8, 255});

    // Holzrahmen
    int rahmen = 30;
    DrawRectangle(0, 0, rahmen, FENSTER_HOEHE, (Color){60, 34, 12, 255});
    DrawRectangle(FENSTER_BREITE - rahmen, 0, rahmen, FENSTER_HOEHE, (Color){60, 34, 12, 255});
    DrawRectangle(0, 0, FENSTER_BREITE, rahmen - 10, (Color){60, 34, 12, 255});
    DrawRectangle(0, FENSTER_HOEHE - rahmen + 10, FENSTER_BREITE, rahmen, (Color){60, 34, 12, 255});

    // Holzmaserung
    for (int i = 0; i < 8; i++) {
        int x = 5 + i * 3;
        DrawLine(x, 0, x, FENSTER_HOEHE, (Color){70, 42, 15, 60});
        DrawLine(FENSTER_BREITE - x, 0, FENSTER_BREITE - x, FENSTER_HOEHE, (Color){70, 42, 15, 60});
    }

    // Einflugsloch unten mittig
    int ex = FENSTER_BREITE / 2;
    int ey = FENSTER_HOEHE - rahmen + 10;
    DrawEllipse(ex, ey, 55, 14, (Color){10, 4, 1, 255});
    DrawEllipseLines(ex, ey, 55, 14, (Color){80, 50, 20, 255});
    DrawText("Eingang", ex - 27, ey - 7, 10, (Color){100, 65, 28, 255});

    // --- Wabenraster ---
    for (int z = 0; z < HEX_ZEILEN; z++) {
        for (int s = 0; s < HEX_SPALTEN; s++) {
            Vector2   mitte = hex_bildschirm(s, z);
            ZellenTyp typ   = spiel->zellen[z * HEX_SPALTEN + s].typ;
            Color     farbe = zellenfarbe(typ);

            DrawPoly(mitte, 6, HEX_RADIUS - 0.5f, 0.0f, farbe);

            // Honigzellen: leichter Glanzeffekt (Wachsdeckel)
            if (typ == ZELLE_HONIG_OFFEN || typ == ZELLE_HONIG_VERD)
                DrawPoly(mitte, 6, HEX_RADIUS * 0.55f, 0.0f, (Color){240, 185, 60, 60});

            DrawPolyLinesEx(mitte, 6, HEX_RADIUS, 0.0f, 1.2f, (Color){18, 9, 2, 55});
        }
    }

    // --- Sim-Bienen im Stock ---
    for (int i = 0; i < spiel->bienen_anzahl; i++) {
        const Biene* b = &spiel->bienen[i];
        if (!b->aktiv) continue;
        // Skip bees that are currently outside the hive
        if (b->bewegung == AUSSENFLUG || b->bewegung == AM_SAMMELN || b->bewegung == HEIMFLUG)
            continue;
        Vector2 pos = {b->pos_x, b->pos_y};
        zeichne_stockbiene(pos, false, spiel->zeit + (float)i * 0.37f);
    }
    // Koenigin: immer in der Mitte
    zeichne_stockbiene(koenigin_pos, true, spiel->zeit);

    // --- Legende ---
    int lx = 42, ly = 60, abst = 16;
    DrawText("BIENENSTOCK", lx, ly - 30, 18, (Color){200, 160, 60, 255});
    struct { ZellenTyp t; const char* bezeichnung; } legende[] = {
        {ZELLE_HONIG_OFFEN, "Honig"},
        {ZELLE_POLLEN,      "Pollen"},
        {ZELLE_BRUT_LARVE,  "Brut"},
        {ZELLE_BRUT_EI,     "Eier"},
        {ZELLE_LEER,        "Leer"},
    };
    for (int i = 0; i < 5; i++) {
        DrawPoly((Vector2){(float)lx + 8, (float)(ly + i * abst + 8)},
                 6, 7, 0.0f, zellenfarbe(legende[i].t));
        DrawText(legende[i].bezeichnung, lx + 20, ly + i * abst, 12,
                 (Color){200, 180, 140, 220});
    }

    // --- HUD: Volk-Informationen ---
    int hx = FENSTER_BREITE - 215, hy = 50;
    DrawText(TextFormat("Volk:    %d Bienen",    spiel->volksgroesse),    hx, hy,      14, (Color){210, 185, 130, 255});
    DrawText(TextFormat("Honig:   %.0f g",  spiel->honig_reif_g),         hx, hy + 18, 14, (Color){210, 150,  20, 255});
    DrawText(TextFormat("Nektar:  %.0f g",  spiel->nektar_unreif_g),      hx, hy + 36, 14, (Color){200, 170,  60, 255});
    DrawText(TextFormat("Pollen:  %.0f g",  spiel->pollen_g),             hx, hy + 54, 14, (Color){200, 185,  40, 255});
    DrawText(TextFormat("Temp:    %.1f C",  spiel->temperatur_c),         hx, hy + 72, 14, (Color){200, 130,  60, 255});
    DrawText(TextFormat("Brut:    %dE %dL %dP", spiel->brut_ei,
             spiel->brut_larve, spiel->brut_puppe),                       hx, hy + 90, 14, (Color){190, 160, 110, 255});

    DrawText("[TAB] Zur Wiesen-Ansicht", FENSTER_BREITE / 2 - 105, FENSTER_HOEHE - 22, 14,
             (Color){140, 110, 60, 200});
}

// ---------------------------------------------------------------------------
void bienenstock_aufraeumen(Spielzustand* spiel) { (void)spiel; }
