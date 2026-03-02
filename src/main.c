#include "raylib.h"
#include "kern/spielzustand.h"
#include "szenen/bienenstock.h"
#include "szenen/wiese.h"
#include "sim/pheromon.h"
#include "sim/haushalt.h"
#include "sim/agent_biene.h"
#include "sim/sim_stock.h"
#include "sim/sim_welt.h"
#include "sim/pfad_stock.h"
#include "sim/pfad_welt.h"
#include "sim/speicher.h"

int main(void) {
    InitWindow(FENSTER_BREITE, FENSTER_HOEHE, "Beehive");
    SetTargetFPS(ZIEL_FPS);

    static Spielzustand spiel = {0};
    spiel.ansicht = ANSICHT_STOCK;

    bienenstock_init(&spiel);
    wiese_init(&spiel);
    pheromon_init(spiel.pheromonfelder);
    haushalt_selbsttest();

    // Außenwelt initialisieren (Trachtquellen, Startzeit, Wetter)
    welt_init(&spiel);

    // Stocksimulation initialisieren (Bienen, Brut, Ressourcen, Auftraege)
    stock_init(&spiel);

    // Flowfields berechnen (einmalig; schnell da BFS auf 64×36 = 2304 Zellen)
    pfad_stock_init();
    pfad_welt_init(&spiel);

    // Load save file automatically if one exists
    if (speicher_vorhanden(SPEICHER_DATEI))
        speicher_lesen(&spiel, SPEICHER_DATEI);

    // Status message shown for 2 seconds after save/load
    float meldung_timer = 0.0f;
    const char* meldung_text = "";

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        // --- Eingabe ---
        // TAB: Ansicht wechseln
        if (IsKeyPressed(KEY_TAB)) {
            spiel.ansicht = (spiel.ansicht == ANSICHT_STOCK)
                            ? ANSICHT_WIESE : ANSICHT_STOCK;
        }
        // P: Pheromon-Debug-Overlay ein/aus
        if (IsKeyPressed(KEY_P))
            spiel.pheromon_debug = !spiel.pheromon_debug;

        // Linke Maustaste: Alarmpheromon an Mausposition abgeben (Debug/Test)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 maus = GetMousePosition();
            pheromon_abgeben(&spiel.pheromonfelder[PHEROMON_ALARM],
                             maus.x, maus.y, 8.0f);
        }
        // S gehalten: Spurpheromon an Mausposition (Debug/Test)
        if (IsKeyDown(KEY_S)) {
            Vector2 maus = GetMousePosition();
            pheromon_abgeben(&spiel.pheromonfelder[PHEROMON_SPUR],
                             maus.x, maus.y, 5.0f);
        }
        // F5: Spielstand speichern
        if (IsKeyPressed(KEY_F5)) {
            if (speicher_schreiben(&spiel, SPEICHER_DATEI)) {
                meldung_text  = "Gespeichert!";
                meldung_timer = 2.0f;
            } else {
                meldung_text  = "Fehler beim Speichern!";
                meldung_timer = 2.0f;
            }
        }
        // F9: Spielstand laden
        if (IsKeyPressed(KEY_F9)) {
            if (speicher_lesen(&spiel, SPEICHER_DATEI)) {
                meldung_text  = "Geladen!";
                meldung_timer = 2.0f;
            } else {
                meldung_text  = "Kein Spielstand gefunden.";
                meldung_timer = 2.0f;
            }
        }

        // --- Simulation ---
        welt_aktualisieren(&spiel, delta);          // Tageszeit, Wetter, Trachtquellen
        pheromon_aktualisieren(spiel.pheromonfelder, delta);
        alle_bienen_aktualisieren(&spiel, delta);
        stock_aktualisieren(&spiel, delta);

        if (spiel.ansicht == ANSICHT_STOCK)
            bienenstock_aktualisieren(&spiel, delta);
        else
            wiese_aktualisieren(&spiel, delta);

        // --- Zeichnen ---
        BeginDrawing();
        if (spiel.ansicht == ANSICHT_STOCK)
            bienenstock_zeichnen(&spiel);
        else
            wiese_zeichnen(&spiel);

        // Pheromon-Overlay (über der Szene, unter HUD)
        if (spiel.pheromon_debug) {
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_ALARM],   PHEROMON_ALARM);
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_SPUR],    PHEROMON_SPUR);
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_NASONOV], PHEROMON_NASONOV);
            DrawText("[P] Pheromon-Debug AN", 10, FENSTER_HOEHE - 44, 13,
                     (Color){255, 255, 100, 200});
            DrawText("LMB=Alarm  S=Spur", 10, FENSTER_HOEHE - 28, 13,
                     (Color){200, 200, 200, 180});
        }

        // Save/load status message (fades after 2 seconds)
        if (meldung_timer > 0.0f) {
            meldung_timer -= delta;
            int alpha = (int)(meldung_timer * 127.0f);
            if (alpha > 255) alpha = 255;
            DrawRectangle(FENSTER_BREITE / 2 - 120, FENSTER_HOEHE / 2 - 20, 240, 40,
                          (Color){0, 0, 0, (unsigned char)(alpha / 2)});
            DrawText(meldung_text,
                     FENSTER_BREITE / 2 - MeasureText(meldung_text, 18) / 2,
                     FENSTER_HOEHE  / 2 - 9,
                     18, (Color){230, 200, 100, (unsigned char)alpha});
        }

        EndDrawing();
    }

    bienenstock_aufraeumen(&spiel);
    wiese_aufraeumen(&spiel);
    CloseWindow();
    return 0;
}
