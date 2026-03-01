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
        {240,  80,  80, 255},   // Rot
        {250, 200,  40, 255},   // Gelb
        {180, 120, 220, 255},   // Lavendel
        {255, 160,  60, 255},   // Orange
        {255, 220, 200, 255},   // Weissrosa
        {100, 200, 120, 255},   // Gruengelb
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
    // Stamm
    DrawRectangle((int)(pos.x - 7), (int)(pos.y + radius * 0.4f),
                  14, (int)(radius * 0.9f), (Color){90, 55, 20, 255});
    // Kronenschatten
    DrawCircleV((Vector2){pos.x + 4, pos.y + 4}, radius, (Color){20, 60, 20, 60});
    // Krone
    DrawCircleV(pos, radius, (Color){38, 110, 42, 255});
    // Lichtreflex
    DrawCircleV((Vector2){pos.x - radius * 0.3f, pos.y - radius * 0.3f},
                radius * 0.45f, (Color){60, 150, 55, 180});
}

static void zeichne_wiesenbiene(Vector2 pos, float animzeit) {
    float flattern = sinf(animzeit * 20.0f) * 2.0f;

    // Fluegel (von oben: zwei Kreise seitlich)
    DrawCircleV((Vector2){pos.x - 6, pos.y - 3 + flattern}, 5, (Color){220, 240, 255, 130});
    DrawCircleV((Vector2){pos.x + 6, pos.y - 3 - flattern}, 5, (Color){220, 240, 255, 130});

    // Koerper (Vogelperspektive: ovale Form)
    DrawEllipse((int)pos.x,     (int)pos.y, 8, 5, (Color){218, 172,   0, 255});
    DrawEllipse((int)pos.x - 1, (int)pos.y, 2, 5, (Color){ 50,  28,   0, 200});
    DrawEllipse((int)pos.x + 3, (int)pos.y, 2, 5, (Color){ 50,  28,   0, 200});
}

// ---------------------------------------------------------------------------
// Wiese zeichnen
// ---------------------------------------------------------------------------
void wiese_zeichnen(const Spielzustand* spiel) {
    ClearBackground((Color){120, 195, 100, 255});

    // Helligkeitsverlauf ueber die Wiese
    for (int i = 0; i < 12; i++) {
        DrawRectangle(0, i * (FENSTER_HOEHE / 12), FENSTER_BREITE, FENSTER_HOEHE / 12,
                      (Color){160, 220, 130, (unsigned char)((18 - i) * 4)});
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

    // Stock-Gebaeude (linke Seite)
    int gx = 50, gy = 280;
    DrawRectangle(gx - 10, gy + 185, 160, 12, (Color){80, 50, 18, 255});
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy + 100, 140, 90}, 0.05f, 4, (Color){140, 90, 35, 255});
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy + 10,  140, 90}, 0.05f, 4, (Color){120, 75, 28, 255});
    DrawRectangleRounded((Rectangle){(float)gx, (float)gy - 20,  140, 30}, 0.05f, 4, (Color){140, 90, 35, 255});
    // Dach
    Vector2 dach[] = {
        {(float)gx - 15, (float)gy - 20},
        {(float)gx + 155, (float)gy - 20},
        {(float)gx + 140, (float)gy - 50},
        {(float)gx + 10,  (float)gy - 50},
    };
    DrawTriangle(dach[0], dach[3], dach[2], (Color){100, 60, 20, 255});
    DrawTriangle(dach[0], dach[2], dach[1], (Color){100, 60, 20, 255});
    // Einflugsloch
    DrawRectangleRounded((Rectangle){(float)gx + 45, (float)gy + 175, 50, 12},
                         0.5f, 4, (Color){20, 10, 2, 255});
    DrawLine(gx, gy + 100, gx + 140, gy + 100, (Color){90, 55, 18, 255});
    DrawLine(gx, gy + 10,  gx + 140, gy + 10,  (Color){90, 55, 18, 255});
    DrawText("Bienenstock", gx + 15, gy - 80, 16, (Color){80, 50, 15, 255});

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
