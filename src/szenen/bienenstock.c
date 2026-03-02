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
        // Honey: deep amber gloss — reference #B85C00–#D4700A
        case ZELLE_HONIG_OFFEN: return (Color){184,  92,   0, 255};
        // Capped honey: matte cream-yellow — reference #E8C84A–#F0D060
        case ZELLE_HONIG_VERD:  return (Color){236, 204,  68, 255};
        // Pollen: vivid orange-yellow — reference #E8820A–#F5A020
        case ZELLE_POLLEN:      return (Color){232, 130,  10, 255};
        // Larva: pearlescent cream-white — reference #F0ECD0–#E8E0B0
        case ZELLE_BRUT_LARVE:  return (Color){238, 228, 196, 255};
        // Egg: very pale ivory
        case ZELLE_BRUT_EI:     return (Color){248, 242, 224, 255};
        // Pupa: warm gold-beige, capped — reference #C8A050–#D4B060
        case ZELLE_BRUT_PUPPE:  return (Color){204, 162,  74, 255};
        // Wax under construction: pale cream-yellow — reference #EDD870–#F5E8A0
        case ZELLE_WACHSBAU:    return (Color){240, 222, 128, 255};
        // Queen area: deep violet
        case ZELLE_KOENIGIN:    return (Color){130,  40, 170, 255};
        // Empty: dark amber depth — reference #7A4A10–#8B5520
        default:                return (Color){ 82,  44,   8, 255};
    }
}

// ---------------------------------------------------------------------------
// Eine Biene im Stock zeichnen
// ---------------------------------------------------------------------------
static void zeichne_stockbiene(Vector2 pos, bool ist_koenigin, float animzeit) {
    float flattern = sinf(animzeit * 22.0f) * 2.5f;

    // Wings: semi-transparent, iridescent tint
    DrawCircleV((Vector2){pos.x - 7, pos.y - 5 + flattern}, 5, (Color){210, 235, 255, 80});
    DrawCircleV((Vector2){pos.x + 7, pos.y - 5 - flattern}, 5, (Color){210, 235, 255, 80});

    if (ist_koenigin) {
        // Queen: elongated, dark violet body with amber head
        DrawEllipse((int)pos.x,      (int)pos.y, 14, 6, (Color){ 48,  18,  64, 255}); // dark body
        DrawEllipse((int)pos.x - 4,  (int)pos.y,  5, 6, (Color){ 90,  38, 110, 255}); // mid segment
        DrawEllipse((int)pos.x + 5,  (int)pos.y,  4, 5, (Color){ 72,  24,  90, 255}); // abdomen tip
        DrawCircle ((int)pos.x + 13, (int)pos.y - 1, 3, (Color){160, 100,  10, 255}); // amber head
    } else {
        // Worker: dark brown body with two subtle amber bands — realistic, not comic-yellow
        DrawEllipse((int)pos.x,     (int)pos.y, 9, 5, (Color){ 42,  24,   6, 255}); // dark brown base
        DrawEllipse((int)pos.x - 1, (int)pos.y, 2, 5, (Color){148,  88,  12, 200}); // amber band 1
        DrawEllipse((int)pos.x + 3, (int)pos.y, 2, 5, (Color){140,  80,  10, 200}); // amber band 2
        DrawCircle ((int)pos.x + 8, (int)pos.y - 1, 2, (Color){ 58,  34,   8, 255}); // head
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
    // Deep hive interior — almost-black warm brown, like inside a wooden box
    ClearBackground((Color){14, 7, 2, 255});
    DrawRectangle(0, 0, FENSTER_BREITE, FENSTER_HOEHE, (Color){28, 14, 4, 255});

    // Wood frame — darker, richer grain
    int rahmen = 30;
    DrawRectangle(0, 0, rahmen, FENSTER_HOEHE, (Color){52, 28, 8, 255});
    DrawRectangle(FENSTER_BREITE - rahmen, 0, rahmen, FENSTER_HOEHE, (Color){52, 28, 8, 255});
    DrawRectangle(0, 0, FENSTER_BREITE, rahmen - 10, (Color){52, 28, 8, 255});
    DrawRectangle(0, FENSTER_HOEHE - rahmen + 10, FENSTER_BREITE, rahmen, (Color){52, 28, 8, 255});

    // Wood grain lines
    for (int i = 0; i < 10; i++) {
        int x = 4 + i * 3;
        DrawLine(x, 0, x, FENSTER_HOEHE, (Color){68, 40, 12, 80});
        DrawLine(FENSTER_BREITE - x, 0, FENSTER_BREITE - x, FENSTER_HOEHE, (Color){68, 40, 12, 80});
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

            // Honey: warm amber gloss centre (light catching liquid surface)
            if (typ == ZELLE_HONIG_OFFEN)
                DrawPoly(mitte, 6, HEX_RADIUS * 0.45f, 0.0f, (Color){220, 140, 20, 70});
            // Capped honey: matte wax sheen
            if (typ == ZELLE_HONIG_VERD)
                DrawPoly(mitte, 6, HEX_RADIUS * 0.60f, 0.0f, (Color){245, 225, 140, 50});

            // Cell walls: dark propolis-brown, clearly visible
            DrawPolyLinesEx(mitte, 6, HEX_RADIUS, 0.0f, 1.5f, (Color){12, 6, 1, 120});
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
