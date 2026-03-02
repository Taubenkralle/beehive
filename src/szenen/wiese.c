#include "wiese.h"
#include <math.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Blumen
// ---------------------------------------------------------------------------
#define MAX_BLUMEN 22

typedef struct {
    Vector2 position;
    Color   bluetenfarbe;
    float   stiel_hoehe;
    float   groesse;
    float   schaukel_versatz;  // Phasenversatz fuer Windanimation
} Blume;

static Blume blumen[MAX_BLUMEN];

// ---------------------------------------------------------------------------
// Baeume
// ---------------------------------------------------------------------------
#define MAX_BAEUME 7

static Vector2 baum_position[MAX_BAEUME];
static float   baum_radius[MAX_BAEUME];

// ---------------------------------------------------------------------------
// Fliegende Bienen (Aussenbereich)
// ---------------------------------------------------------------------------
#define MAX_WIESENBIENEN 18

typedef struct {
    Vector2 position;
    Vector2 ziel;
    float   geschwindigkeit;
    float   animzeit;
    bool    heimwaerts;    // true = Richtung Stock, false = Richtung Blume
} WiesenBiene;

static WiesenBiene wbienen[MAX_WIESENBIENEN];

static const Vector2 STOCK_POSITION = {130.0f, 370.0f};

// ---------------------------------------------------------------------------
// Initialisierung
// ---------------------------------------------------------------------------
void wiese_init(Spielzustand* spiel) {
    (void)spiel;

    Color bluetenpalette[] = {
        {196,  32,  32, 255},   // Kraeftiges Rot  #C42020
        {228,  56,  56, 255},   // Leuchtendes Rot #E43838
        {240, 160,  20, 255},   // Sattes Orange
        {160,  60, 210, 255},   // Tiefes Lavendel
        {255, 200,  40, 255},   // Sonniges Gelb
        { 80, 180,  80, 255},   // Wiesengruen
    };

    for (int i = 0; i < MAX_BLUMEN; i++) {
        blumen[i].position.x      = 260.0f + (float)(rand() % 950);
        blumen[i].position.y      = 80.0f  + (float)(rand() % 560);
        blumen[i].bluetenfarbe    = bluetenpalette[rand() % 6];
        blumen[i].stiel_hoehe     = 18.0f + (float)(rand() % 22);
        blumen[i].groesse         = 10.0f + (float)(rand() % 12);
        blumen[i].schaukel_versatz = (float)(rand() % 628) * 0.01f;
    }

    int bpos[][2] = {{900,120},{1100,220},{1050,580},{820,650},{350,80},{420,620},{1180,420}};
    for (int i = 0; i < MAX_BAEUME; i++) {
        baum_position[i] = (Vector2){(float)bpos[i][0], (float)bpos[i][1]};
        baum_radius[i]   = 44.0f + (float)(rand() % 24);
    }

    for (int i = 0; i < MAX_WIESENBIENEN; i++) {
        wbienen[i].position        = STOCK_POSITION;
        wbienen[i].ziel            = blumen[rand() % MAX_BLUMEN].position;
        wbienen[i].geschwindigkeit = 60.0f + (float)(rand() % 60);
        wbienen[i].animzeit        = (float)(rand() % 100) * 0.1f;
        wbienen[i].heimwaerts      = false;
    }
}

// ---------------------------------------------------------------------------
// Aktualisierung pro Frame
// ---------------------------------------------------------------------------
void wiese_aktualisieren(Spielzustand* spiel, float delta) {
    // zeit is managed by welt_aktualisieren; only update local animation state here
    (void)spiel;

    for (int i = 0; i < MAX_WIESENBIENEN; i++) {
        wbienen[i].animzeit += delta;

        Vector2 richtung = {
            wbienen[i].ziel.x - wbienen[i].position.x,
            wbienen[i].ziel.y - wbienen[i].position.y,
        };
        float abstand = sqrtf(richtung.x * richtung.x + richtung.y * richtung.y);

        if (abstand < 12.0f) {
            if (wbienen[i].heimwaerts) {
                // Am Stock angekommen → zur naechsten Blume
                wbienen[i].ziel       = blumen[rand() % MAX_BLUMEN].position;
                wbienen[i].heimwaerts = false;
            } else {
                // An Blume angekommen → nach Hause
                wbienen[i].ziel       = STOCK_POSITION;
                wbienen[i].heimwaerts = true;
            }
        } else {
            // Leichter Schlenker quer zur Flugrichtung (naturgetreu)
            float schwung = sinf(wbienen[i].animzeit * 4.0f) * 8.0f;
            float nx      = -richtung.y / abstand;
            float ny      =  richtung.x / abstand;
            float schritt = wbienen[i].geschwindigkeit * delta;
            wbienen[i].position.x += (richtung.x / abstand) * schritt + nx * schwung * delta;
            wbienen[i].position.y += (richtung.y / abstand) * schritt + ny * schwung * delta;
        }
    }
}

// ---------------------------------------------------------------------------
// Zeichnen-Hilfsfunktionen
// ---------------------------------------------------------------------------
static void zeichne_blume(const Blume* b, float zeit) {
    float schwung = sinf(zeit * 1.2f + b->schaukel_versatz) * 3.0f;

    // Stiel
    DrawLineEx((Vector2){b->position.x, b->position.y},
               (Vector2){b->position.x + schwung, b->position.y - b->stiel_hoehe},
               2.0f, (Color){50, 120, 40, 255});

    Vector2 spitze = {b->position.x + schwung, b->position.y - b->stiel_hoehe};

    // 6 Blaetter um den Mittelpunkt
    for (int k = 0; k < 6; k++) {
        float winkel = k * (3.14159f / 3.0f) + zeit * 0.3f;
        Vector2 blatt = {
            spitze.x + cosf(winkel) * b->groesse * 0.85f,
            spitze.y + sinf(winkel) * b->groesse * 0.85f,
        };
        DrawCircleV(blatt, b->groesse * 0.55f, b->bluetenfarbe);
    }

    // Bluetenherz
    DrawCircleV(spitze, b->groesse * 0.40f, (Color){240, 210, 40, 255});
}

static void zeichne_baum(Vector2 pos, float radius) {
    // Trunk — dark bark
    DrawRectangle((int)(pos.x - 7), (int)(pos.y + radius * 0.4f),
                  14, (int)(radius * 0.9f), (Color){58, 34, 10, 255});
    // Crown shadow for depth
    DrawCircleV((Vector2){pos.x + 5, pos.y + 5}, radius, (Color){10, 36, 10, 90});
    // Main crown — deep forest green
    DrawCircleV(pos, radius, (Color){28, 82, 28, 255});
    // Secondary crown layer
    DrawCircleV((Vector2){pos.x - 6, pos.y - 4}, radius * 0.75f, (Color){34, 96, 34, 255});
    // Light catch highlight
    DrawCircleV((Vector2){pos.x - radius * 0.32f, pos.y - radius * 0.32f},
                radius * 0.38f, (Color){52, 128, 44, 200});
}

static void zeichne_wiesenbiene(Vector2 pos, float animzeit) {
    float flattern = sinf(animzeit * 20.0f) * 2.0f;

    // Wings — iridescent, semi-transparent
    DrawCircleV((Vector2){pos.x - 6, pos.y - 4 + flattern}, 5, (Color){200, 228, 255, 100});
    DrawCircleV((Vector2){pos.x + 6, pos.y - 4 - flattern}, 5, (Color){200, 228, 255, 100});

    // Body — dark brown base with amber bands (top-down view)
    DrawEllipse((int)pos.x,     (int)pos.y, 8, 5, (Color){ 40,  22,   5, 255}); // dark brown
    DrawEllipse((int)pos.x - 1, (int)pos.y, 2, 5, (Color){148,  88,  12, 210}); // amber band
    DrawEllipse((int)pos.x + 3, (int)pos.y, 2, 5, (Color){136,  78,  10, 210}); // amber band
}

// ---------------------------------------------------------------------------
// Wiese zeichnen
// ---------------------------------------------------------------------------
void wiese_zeichnen(const Spielzustand* spiel) {
    // Rich meadow green — reference #3A7A28–#4A8C2A
    ClearBackground((Color){52, 108, 36, 255});

    // Brightness gradient: lighter sky-green at top, deeper grass at bottom
    for (int i = 0; i < 12; i++) {
        DrawRectangle(0, i * (FENSTER_HOEHE / 12), FENSTER_BREITE, FENSTER_HOEHE / 12,
                      (Color){80, 160, 50, (unsigned char)((14 - i) * 5)});
    }

    // Feldweg vom Stock zur Wiesenflaeche
    DrawRectangle(130, 330, 380, 80, (Color){160, 120, 70, 180});
    DrawRectangle(510, 340, 200, 60, (Color){150, 112, 65, 120});

    // Baeume (hinter Blumen)
    for (int i = 0; i < MAX_BAEUME; i++)
        zeichne_baum(baum_position[i], baum_radius[i]);

    // Teich
    DrawEllipse(720, 580, 55, 32, (Color){80, 160, 210, 200});
    DrawEllipseLines(720, 580, 55, 32, (Color){60, 130, 180, 255});
    DrawEllipse(705, 570, 18, 8, (Color){180, 220, 255, 90});

    // Blumen
    for (int i = 0; i < MAX_BLUMEN; i++)
        zeichne_blume(&blumen[i], spiel->zeit);

    // Stock-Gebaeude — organic layered hive, dark amber-black palette
    // Reference: hive.jpeg — #1A0F0A (outer), #3D2010 (mid), #8B5E2A (highlight)
    int gx = 45, gy = 260;

    // Ground platform / Bodenbrett
    DrawRectangle(gx - 12, gy + 210, 168, 10, (Color){46, 26, 8, 255});
    DrawRectangle(gx - 8,  gy + 205, 160, 6,  (Color){60, 36, 10, 255});

    // Three stacked hive bodies — each slightly different shade
    // Bottom box (darkest)
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy + 140, 144, 66}, 0.08f, 6, (Color){ 38, 20,  6, 255});
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy + 140, 144, 66}, 0.08f, 6, (Color){ 60, 34, 10,  40});
    // Middle box
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy +  72, 144, 68}, 0.08f, 6, (Color){ 50, 28,  8, 255});
    // Top box (slightly lighter)
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy +   8, 144, 64}, 0.08f, 6, (Color){ 58, 32, 10, 255});

    // Amber highlights on box edges (propolis-stained wood)
    DrawRectangleLines(gx,     gy + 140, 144, 66, (Color){100, 60, 18, 120});
    DrawRectangleLines(gx,     gy +  72, 144, 68, (Color){ 92, 54, 16, 120});
    DrawRectangleLines(gx,     gy +   8, 144, 64, (Color){ 84, 48, 14, 120});

    // Inner warmth glow — amber visible through hive body
    DrawRectangleRounded((Rectangle){(float)gx + 20, (float)gy + 90, 104, 110}, 0.15f, 4, (Color){140, 80, 18, 18});

    // Roof — dark trapezoid, slightly overhanging
    Vector2 dach[] = {
        {(float)gx - 16, (float)gy + 10},
        {(float)gx + 160, (float)gy + 10},
        {(float)gx + 148, (float)gy - 32},
        {(float)gx -   4, (float)gy - 32},
    };
    DrawTriangle(dach[0], dach[3], dach[2], (Color){26, 14, 4, 255});
    DrawTriangle(dach[0], dach[2], dach[1], (Color){26, 14, 4, 255});
    // Roof highlight edge
    DrawLineEx((Vector2){dach[3].x, dach[3].y}, (Vector2){dach[2].x, dach[2].y},
               2.0f, (Color){80, 48, 14, 180});
    DrawLineEx((Vector2){dach[0].x, dach[0].y}, (Vector2){dach[1].x, dach[1].y},
               2.0f, (Color){72, 42, 12, 180});

    // Entrance slot
    DrawRectangleRounded((Rectangle){(float)gx + 40, (float)gy + 196, 64, 10},
                         0.5f, 4, (Color){8, 4, 1, 255});
    DrawRectangleRoundedLines((Rectangle){(float)gx + 40, (float)gy + 196, 64, 10},
                               0.5f, 4, (Color){80, 48, 14, 160});

    // Label
    DrawText("Bienenstock", gx + 10, gy - 60, 14, (Color){90, 58, 18, 200});

    // Trachtquellen (sim data)
    for (int i = 0; i < spiel->quellen_anzahl; i++) {
        const Trachtquelle* q = &spiel->quellen[i];
        if (!q->aktiv) continue;
        Vector2 pos = {q->pos_x, q->pos_y};
        Color   farbe;
        const char* label;
        switch (q->typ) {
            case TRACHT_NEKTAR:   farbe = (Color){230, 120,  20, 200}; label = "N"; break;
            case TRACHT_POLLEN:   farbe = (Color){220, 200,  30, 200}; label = "P"; break;
            case TRACHT_WASSER:   farbe = (Color){ 80, 160, 210, 200}; label = "W"; break;
            default:              farbe = (Color){150, 100,  60, 200}; label = "?"; break;
        }
        float fuellgrad = (q->kapazitaet_g > 0.0f) ? (q->vorrat_g / q->kapazitaet_g) : 0.0f;
        DrawCircleV(pos, 18.0f, (Color){farbe.r, farbe.g, farbe.b, 60});
        DrawCircleV(pos, 18.0f * fuellgrad, farbe);
        DrawCircleLines((int)pos.x, (int)pos.y, 18, (Color){farbe.r, farbe.g, farbe.b, 200});
        DrawText(label, (int)pos.x - 4, (int)pos.y - 6, 14, (Color){255, 255, 255, 220});
    }

    // Wiesenbienen (decorative)
    for (int i = 0; i < MAX_WIESENBIENEN; i++)
        zeichne_wiesenbiene(wbienen[i].position, wbienen[i].animzeit);

    // Sim-Bienen im Aussenbereich
    for (int i = 0; i < spiel->bienen_anzahl; i++) {
        const Biene* b = &spiel->bienen[i];
        if (!b->aktiv) continue;
        if (b->bewegung != AUSSENFLUG && b->bewegung != AM_SAMMELN && b->bewegung != HEIMFLUG)
            continue;
        Vector2 pos = {b->pos_x, b->pos_y};
        zeichne_wiesenbiene(pos, spiel->zeit + (float)i * 0.41f);
    }

    // HUD
    DrawText("WIESEN-ANSICHT", 14, 14, 20, (Color){40, 80, 30, 230});
    // Tageszeit als Uhr (0.0=0:00, 0.5=12:00, 1.0=24:00)
    int stunde = (int)(spiel->tageszeit * 24.0f) % 24;
    int minute = (int)(spiel->tageszeit * 24.0f * 60.0f) % 60;
    DrawText(TextFormat("%02d:%02d  Temp: %.0f C  Wind: %.0f%%",
             stunde, minute, spiel->aussentemp_c, spiel->wind_staerke * 100.0f),
             14, 40, 13, (Color){60, 100, 50, 220});
    if (spiel->regen)
        DrawText("REGEN", 14, 58, 13, (Color){ 80, 140, 200, 220});
    if (spiel->flug_gesperrt)
        DrawText("FLUG GESPERRT", 14, 58, 13, (Color){200,  80,  40, 220});
    DrawText(TextFormat("Honig: %.0fg  Nektar: %.0fg  Pollen: %.0fg",
             spiel->honig_reif_g, spiel->nektar_unreif_g, spiel->pollen_g),
             14, FENSTER_HOEHE - 44, 13, (Color){60, 100, 40, 200});
    DrawText("[TAB] Zur Stock-Ansicht", FENSTER_BREITE / 2 - 100, FENSTER_HOEHE - 26, 14,
             (Color){50, 90, 40, 200});
}

// ---------------------------------------------------------------------------
void wiese_aufraeumen(Spielzustand* spiel) { (void)spiel; }
